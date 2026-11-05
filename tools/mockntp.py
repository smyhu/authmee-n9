import socket,struct,time,threading
E1900=2208988800
def ntp():
    s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM); s.bind(('127.0.0.1',123))
    while True:
        d,a=s.recvfrom(100); t=time.time()+5.0
        sec=int(t+E1900); fr=int((t%1)*2**32)
        p=bytearray(48); p[0]=0x1c; p[1]=2
        p[32:40]=struct.pack('>II',sec,fr); p[40:48]=struct.pack('>II',sec,fr)
        s.sendto(bytes(p),a)
def tcp():
    s=socket.socket(); s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1); s.bind(('127.0.0.1',37)); s.listen(5)
    while True:
        c,a=s.accept(); c.send(struct.pack('>I',int(time.time()-3+E1900))); c.close()
threading.Thread(target=tcp,daemon=True).start(); ntp()
