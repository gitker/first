
package main

import (
	"net"
	"io"
	"os"
	"log"
	"encoding/binary"
	"io/ioutil"
	"strings"
	"encoding/json"
	"os/signal"
	"syscall"
)

func logf(f string, v ...interface{}) {

	log.Printf(f, v...)

}

type Cmd struct{
	cmd string
	path string
}

//get/put len path 
func getCmd(r io.Reader)(*Cmd,error){
	 b :=make([]byte,260)
	 res := &Cmd{}

	 logf("0")

	 _, err := io.ReadFull(r, b[:3]) 
	if err != nil {
		return nil, err
	}
	logf("1")
	if b[0]=='g' &&b[1] =='e'&&b[2]=='t'{
		res.cmd="get"
	}else if b[0]=='p' &&b[1] =='u'&&b[2]=='t'{
		res.cmd="put"
	}else{
		return nil, err
	}
	logf("2")

	_, err = io.ReadFull(r, b[3:4]) // read 2nd byte for domain length
	if err != nil {
			return nil, err
	}
	logf("3")
	_, err = io.ReadFull(r, b[4:4+int(b[3])])
	if err != nil {
		return nil, err
	}
	res.path = string(b[4:4+int(b[3])])

	return res,nil

}

func processGet(path string,c net.Conn)(error){
   fileInfo, err := os.Stat(path)
   if (err != nil){
	   return err
   }
   if fileInfo.IsDir(){
	   return err
   }
   filesize:= fileInfo.Size()
   b := make([]byte, 8)
   binary.LittleEndian.PutUint64(b, uint64(filesize))
   if _, err = c.Write(b); err != nil {
	logf("failed to send file len: %v", err)
	return nil
  }

   fi, err := os.Open(path)
    if err != nil {
        return err
    }
	defer fi.Close()
	buf := make([]byte, 4096)

	for{
		n,err := fi.Read(buf)

		if err != nil && err != io.EOF {
            return err
		}
		if 0==n{
			break
		}
		if _, err = c.Write(buf[0:n]); err != nil {
			logf("failed to send file : %v", err)
			return err
		}
	}

   return nil
}

func processPut(path string,c net.Conn)(error){
	f, err := os.Create(path)
	if err!=nil {
		return err
	}
	defer f.Close()
	b := make([]byte, 8)

	_, err = io.ReadFull(c,b)
	if err != nil {
		return  err
	}
	fsize := int64(binary.LittleEndian.Uint64(b))
	println("put size is ",fsize)

	buf := make([]byte, 4096)
	geted := int64(0)
	progress := int64(1)
	for{
		n,err := c.Read(buf)

		if err != nil && err != io.EOF {
            return err
		}
		if 0==n{
			break
		}
		geted = geted + int64(n)
		pgs := (geted*100/fsize)
		if pgs >progress{
			println(pgs)
		}
		for pgs > progress{
			progress = progress +1
		}


		if _, err = f.Write(buf[0:n]); err != nil {
			logf("failed to write file : %v", err)
			return err
		}
	}
	return nil

}

func setCmd(req Cmd,c net.Conn)(bool,error){
	if _, err := c.Write([]byte(req.cmd)); err != nil {
		logf("failed to write cmd : %v", err)
		return false,err
	}
	plen := len(req.path)
	if plen > 255{
		return false,nil
	}
	if plen <16{
		req.path = req.path + "                "
	}
	plen = len(req.path)
	b := []byte{byte(plen)}
	if _, err := c.Write(b); err != nil {
		logf("failed to write plen : %v", err)
		return false,err
	}
	if _, err := c.Write([]byte(req.path)); err != nil {
		logf("failed to write path : %v", err)
		return false,err
	}
	return true,nil

}

func tcpLocal(server string ,req Cmd,shade func(net.Conn) net.Conn){
	rc, err := net.Dial("tcp", server)
	if err != nil {
		logf("failed to connect to server %v: %v", server, err)
		return
	}
	defer rc.Close()
	rc.(*net.TCPConn).SetKeepAlive(true)
//	rc = shade(rc)
	res,err := setCmd(req,rc)
	if res != true{
		return 
	}
	logf("set cmd success:")
	if req.cmd == "get"{
		processPut(req.path,rc)
	}else{
		processGet(req.path,rc)
	}

}

func tcpRemote(addr string,shade func(net.Conn) net.Conn){
	l, err := net.Listen("tcp", addr)
	if err != nil {
		logf("failed to listen on %s: %v", addr, err)
		return
	}

	logf("listening TCP on %s", addr)

	for{
		c, err := l.Accept()
		if err != nil {
			logf("failed to accept: %v", err)
			continue
		}
		logf(" accepted")

		go func(){
			defer c.Close()
			c.(*net.TCPConn).SetKeepAlive(true)
			//c = shade(c)

			req,err := getCmd(c)
			if (req == nil){
				logf("failed to get cmd: %v", err)
				return
			}
			logf("get cmd ok")
			if req.cmd == "get"{
				processGet(req.path,c)
			}else{
				processPut(req.path,c)
			}

		}()
	}

}
type cfg struct {
	ServerAddr string
	ServerPort string
	LocalPort  string
	Password   string
}

func main() {

	var parms cfg

	data, err := ioutil.ReadFile("./config.json")
	if err != nil {
		println("can't find cfg ")
		return
	}

	err = json.Unmarshal(data, &parms)

	if err != nil {
		println("cfg error")
		return
	}

	
	
	if strings.Contains(os.Args[0], "ser") {
		go tcpRemote("0.0.0.0:"+parms.LocalPort,nil)
		sigCh := make(chan os.Signal, 1)
	    signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)
	    <-sigCh
	} else {
		c := Cmd{}
		if len(os.Args) <3{
			println("format error")
			return
		}
		if strings.Contains(os.Args[1], "g"){
			c.cmd = "get"
		}else{
			c.cmd ="put"
		}
		c.path = os.Args[2]
		tcpLocal(parms.ServerAddr+":"+parms.LocalPort, c,nil)
	}

}