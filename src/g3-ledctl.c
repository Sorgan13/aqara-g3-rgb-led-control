/*
 * g3-ledctl: experimental Aqara Camera Hub G3 indicator control via ha_agent.
 *
 * Independent, freestanding ARM Linux EABI executable using raw syscalls.
 * No libc, dynamic linker, network service, firmware modification, or reboot.
 *
 * Protocol reference: https://github.com/stackia/aqara-agent2mqtt
 * LED resource IDs inferred from AqaraPOST-Homeassistant (cloud API):
 *     on  14.21.85 = 5162041
 *     off 14.21.85 = 2212920
 * IMPORTANT: Whether those values work through /lumi/gw/res/write locally
 * remains unverified! This is experimental, and replies must be inspected.
 *
 * Usage: g3-ledctl probe
 *        g3-ledctl on  lumi1.xxxxxxxxxxxx
 *        g3-ledctl off lumi1.xxxxxxxxxxxx
 */

#define AF_UNIX 1
#define SOCK_SEQPACKET 5
#define POLLIN 1
#define SYS_EXIT 1
#define SYS_WRITE 4
#define SYS_CLOSE 6
#define SYS_POLL 168
#define SYS_SOCKET 281
#define SYS_CONNECT 283
#define SYS_SENDTO 290
#define SYS_RECVFROM 292

static inline long call1(long n, long a) {
    register long r0 __asm__("r0") = a;
    register long r7 __asm__("r7") = n;
    __asm__ volatile("svc #0" : "+r"(r0) : "r"(r7) : "memory", "cc");
    return r0;
}
static inline long call3(long n, long a, long b, long c) {
    register long r0 __asm__("r0") = a;
    register long r1 __asm__("r1") = b;
    register long r2 __asm__("r2") = c;
    register long r7 __asm__("r7") = n;
    __asm__ volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r7) : "memory", "cc");
    return r0;
}
static inline long call6(long n, long a, long b, long c, long d, long e, long f) {
    register long r0 __asm__("r0") = a;
    register long r1 __asm__("r1") = b;
    register long r2 __asm__("r2") = c;
    register long r3 __asm__("r3") = d;
    register long r4 __asm__("r4") = e;
    register long r5 __asm__("r5") = f;
    register long r7 __asm__("r7") = n;
    __asm__ volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5), "r"(r7) : "memory", "cc");
    return r0;
}

static unsigned slen(const char *s) { unsigned n=0; while(s[n]) n++; return n; }
static int eq(const char *a,const char *b) { while(*a && *b && *a==*b) {a++;b++;} return *a==*b; }
static void out(const char *s) { call3(SYS_WRITE,1,(long)s,slen(s)); }
static void err(const char *s) { call3(SYS_WRITE,2,(long)s,slen(s)); }
static void putlen(const char *s,unsigned len) {call3(SYS_WRITE,1,(long)s,len);}
static void putnum(long n) {
    char tmp[24]; unsigned i=0; int neg=(n<0);
    unsigned long x=neg ? (unsigned long)(-(n+1))+1 : (unsigned long)n;
    do {tmp[i++]=(char)('0'+x%10);x/=10;}while(x);
    if(neg)tmp[i++]='-';
    for(unsigned a=0;a<i/2;a++){char t=tmp[a];tmp[a]=tmp[i-1-a];tmp[i-1-a]=t;}
    putlen(tmp,i);
}
struct sockaddr_un_local {unsigned short family;char path[108];};
struct pollfd_local {int fd; short events; short revents;};
static int sendstr(int fd,const char *s){return (int)call6(SYS_SENDTO,fd,(long)s,slen(s),0,0,0);}
static int append(char *dst,unsigned cap,unsigned *pos,const char *s){
    while(*s){if(*pos+1>=cap)return -1;dst[(*pos)++]=*s++;}
    dst[*pos]=0;return 0;
}
static int valid_did(const char *s){
    if(slen(s)<8||slen(s)>80)return 0;
    if(!(s[0]=='l'&&s[1]=='u'&&s[2]=='m'&&s[3]=='i'&&s[4]=='1'&&s[5]=='.'))return 0;
    for(unsigned i=6;s[i];i++){
        char c=s[i];
        if(!((c>='0'&&c<='9')||(c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='.'))return 0;
    }
    return 1;
}

static int run(int argc,char **argv){
    if(argc<2 || (!eq(argv[1],"probe") && !eq(argv[1],"on") && !eq(argv[1],"off"))){
        out("Usage: g3-ledctl probe | on lumi1.xxxxx | off lumi1.xxxxx\n");return 2;
    }
    int probe=eq(argv[1],"probe");
    if(!probe && (argc!=3 || !valid_did(argv[2]))){
        err("Error: provide Aqara G3 DID as lumi1.xxxxx (not IP/MAC with separators).\n");return 2;
    }
    int fd=(int)call3(SYS_SOCKET,AF_UNIX,SOCK_SEQPACKET,0);
    if(fd<0){err("socket() failed: ");putnum(fd);out("\n");return 3;}
    struct sockaddr_un_local sa;sa.family=AF_UNIX;
    const char *path="/tmp/miio_agent.socket";
    unsigned i;for(i=0;path[i];i++)sa.path[i]=path[i];sa.path[i]=0;
    int connected=(int)call3(SYS_CONNECT,fd,(long)&sa,2+i+1);
    if(connected<0){err("connect() failed: ");putnum(connected);out("\n");call1(SYS_CLOSE,fd);return 4;}
    out("Connected to ha_agent SOCK_SEQPACKET socket.\n");
    /* `bind` is client registration in ha_agent's application protocol.
       It is not the POSIX `bind()` system call. */
    const char *bindmsg="{\"address\":256,\"method\":\"bind\"}";
    if(sendstr(fd,bindmsg)<0){err("Cannot register client.\n");call1(SYS_CLOSE,fd);return 5;}
    if(!probe){
        const char *pre="{\"_to\":4,\"id\":123,\"method\":\"auto.control\",\"params\":{\"name\":\"/lumi/gw/res/write\",\"value\":{\"data\":{\"14.21.85\":\"";
        const char *val=eq(argv[1],"on")?"5162041":"2212920";
        const char *mid="\"},\"did\":\"";
        const char *post="\",\"source\":\"\"}}}";
        char msg[512];unsigned n=0;
        if(append(msg,sizeof(msg),&n,pre)<0 || append(msg,sizeof(msg),&n,val)<0 || append(msg,sizeof(msg),&n,mid)<0 || append(msg,sizeof(msg),&n,argv[2])<0 || append(msg,sizeof(msg),&n,post)<0){err("JSON buffer overflow.\n");call1(SYS_CLOSE,fd);return 6;}
        if(sendstr(fd,msg)<0){err("Cannot send LED command.\n");call1(SYS_CLOSE,fd);return 7;}
        out("Sent experimental LED ");out(argv[1]);out(" command via /lumi/gw/res/write.\n");
        out("This only confirms the packet was SENT, not LED success.\n");
    }
    /* Wait briefly for a broker reply. Do not block the camera shell. */
    for(int k=0;k<6;k++){
        struct pollfd_local p={fd,POLLIN,0};
        long ready=call3(SYS_POLL,(long)&p,1,k==0?1000:200);
        if(ready<=0)break;
        char response[4096];
        long n=call6(SYS_RECVFROM,fd,(long)response,sizeof(response)-1,0,0,0);
        if(n<=0)break;
        response[n]=0;
        out("ha_agent reply: ");putlen(response,(unsigned)n);out("\n");
    }
    call1(SYS_CLOSE,fd);
    if(probe)out("Probe complete; no LED command sent.\n");
    return 0;
}

void program_start(unsigned int *stack) {
    int argc=(int)stack[0];
    char **argv=(char **)(stack+1);
    int status=run(argc,argv);
    call1(SYS_EXIT,status);
    __builtin_unreachable();
}

__attribute__((naked,noreturn)) void _start(void) {
    __asm__ volatile("mov r0, sp\nbl program_start\nb .");
}
