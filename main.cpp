#include <sys/socket.h>
#include <arpa/inet.h>
# include <netdb.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <sys/ioctl.h>
# include <net/if.h>
#include <netinet/tcp.h>
# include <netinet/in.h>
#include <stdio.h>
# include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "fixedstring.inl"
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>

///////////////////////////////////////////////////////////////////////////////

struct scpisock
{
    typedef std::vector<char> buffer_t;

    scpisock(const char* addr, const char* port);
    ~scpisock();

    int recv(int max);
    bool do_cmd( const std::string& cmd );
    bool cmd( const char*fmt, ... );
    bool query( const char*fmt, ... );
    const char* c_str() { return & the_buf[0]; }
    buffer_t the_buf;
    int _sock;

};

///////////////////////////////////////////////////////////////////////////////

scpisock::~scpisock()
{
    auto cl = close(_sock);
    assert(cl>=0);
}

///////////////////////////////////////////////////////////////////////////////

scpisock::scpisock(const char* addr, const char* port)
{
    addrinfo addr_hints;
    memset(&addr_hints,0,sizeof(struct addrinfo));

    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_family = AF_INET;
    addr_hints.ai_flags = INADDR_ANY;

    addrinfo* result = nullptr;

    int retval = getaddrinfo(addr,port,&addr_hints,&result);
    assert(retval>=0);

    _sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    printf( "S<%d>\n", _sock );

    // Establish TCP connection
    auto con = ::connect(_sock, result->ai_addr, result->ai_addrlen);

    assert(con>=0);

    int StateNODELAY = 1; // Turn NODELAY on

    setsockopt(_sock,IPPROTO_TCP,TCP_NODELAY, (void *)&StateNODELAY,sizeof StateNODELAY);

}

///////////////////////////////////////////////////////////////////////////////

bool scpisock::do_cmd( const std::string& buf )
{
    std::string b2 =buf+"\n";
    int len = b2.length();
    //printf( "send cmd<%s> len<%d>\n", buf.c_str(), len );
    bool rval = (send(_sock,b2.c_str(),len,0)>=0);
    return rval;
}

bool scpisock::cmd( const char*fmt, ... )
{
    char buffer[256];

    va_list argp;
    va_start(argp, fmt);
    vsnprintf( & buffer[0], 256, fmt, argp);
    va_end(argp);
    int len = strlen(buffer);
    printf( "send cmd<%s> len<%d>\n", buffer, len );
    bool ok = do_cmd(buffer);
    do_cmd("*wai\n");
    usleep(500000);
    return ok;
}


bool scpisock::query( const char*fmt, ... )
{
    char buffer[256];

    va_list argp;
    va_start(argp, fmt);
    vsnprintf( & buffer[0], 256, fmt, argp);
    va_end(argp);
    int len = strlen(buffer);
    printf( "send cmd<%s> len<%d>\n", buffer, len );
    bool ok = (send(_sock,buffer,len,0)>=0);

    usleep(1<<19);

    int rsiz = recv(200);
    the_buf.resize(rsiz+1);
    the_buf[rsiz] = 0;
    printf( "query<%s> res<%s>\n", buffer, & the_buf[0] );
    return ok;
}

///////////////////////////////////////////////////////////////////////////////

int scpisock::recv(int max)
{
    the_buf.resize(max);

    int actual = ::recv(_sock,&the_buf[0],max,0);
    if( actual>0 && actual+1<max )
        the_buf.resize(actual);
    //printf( "recv max<%d> act<%d>\n", max, actual );
    return actual;
}

///////////////////////////////////////////////////////////////////////////////

static inline void loadBar(int val, int of_n, int gran, int width)
{
    // Only update r times.
 
    // Calculuate the ratio of complete-to-incomplete.
    float ratio = val/(float)of_n;
    int   c     = ratio * width;
 
    // Show the percentage complete.
    printf("%3d%% [", (int)(ratio*100) );
 
    // Show the load bar.
    for (int x=0; x<c; x++)
       printf("=");
 
    for (int x=c; x<width; x++)
       printf(" ");
 
    // ANSI Control codes to go back to the
    // previous line and clear it.
    printf("]\n\033[F\033[J");
    //printf("]\n33[1A33[2K");
   // fflush(stdout);
//    printf( "yo\n");
}

int main( int argc, const char** argv )
{
    // Send SCPI command

    if( argc != 2 )
    {
        printf( "usage:\n" );
        printf( " rigol.exe prep    : setup oscope params\n");
        printf( " rigol.exe cap     : capture oscope frame\n");
        exit(0);
    }

    scpisock scpio("10.0.1.75", "5555");

    bool ok = scpio.cmd("*IDN?\n");
    assert(ok);
    scpio.recv(200);
    printf("Instrument ID: %s\n", scpio.c_str() );

    if( 0 == strcmp(argv[1], "prep") )
    {
        scpio.cmd(":disp:type vect");

        scpio.cmd(":acq:type aver");
        scpio.cmd(":acq:mdep auto");
        scpio.cmd(":acq:aver 256");

        scpio.cmd(":trig:mode edge");
        scpio.cmd(":trig:edg:sour chan1");
        scpio.cmd(":trig:edg:slop pos");
        scpio.cmd(":trig:edg:lev 1.5");
        scpio.cmd(":tim:scal 0.00005"); // 0.0002==200us
        scpio.cmd(":tim:offs 0.0"); // 0.0002==200us

        scpio.cmd(":chan1:disp on");
        scpio.cmd(":chan2:disp on");
        scpio.cmd(":chan3:disp on");

        scpio.cmd(":chan1:coup dc");
        scpio.cmd(":chan2:coup dc");
        scpio.cmd(":chan3:coup dc");

        scpio.cmd(":chan1:tcal 0.0");
        scpio.cmd(":chan2:tcal 0.0");
        scpio.cmd(":chan3:tcal 0.0");

        scpio.cmd(":chan1:offs 0.0");
        scpio.cmd(":chan2:offs -0.5");
        scpio.cmd(":chan3:offs -1.0");

        scpio.cmd(":chan1:scal 1.0");
        scpio.cmd(":chan2:scal 1.0");
        scpio.cmd(":chan3:scal 1.0");

        scpio.cmd(":chan1:prob 1");
        scpio.cmd(":chan2:prob 1");
        scpio.cmd(":chan3:prob 1");
    }
    else if( 0 == strcmp( argv[1], "cap") )
    {
        ok = scpio.cmd(":disp:data?");
        assert(ok);
        scpio.recv(11);
        printf("disptmch: %s\n", scpio.c_str() );
        int bmsize = atoi(scpio.c_str()+4);
        printf("dispsize: %d\n", bmsize );

        int tot = 0;
        FILE* outf = fopen( "captures/out.bmp", "wb");
        while(tot<bmsize)
        {
            int siz = scpio.recv(bmsize);
            if( (tot+siz)>bmsize )
            {
                siz -= ((tot+siz)-bmsize);
            }
            tot += siz;
            loadBar( tot, bmsize, 32, 80 );
            fwrite(scpio.c_str(),siz,1,outf);
            //printf( "recv<%d> tot<%d>\r", siz, tot );
        }
        fclose(outf);

        system( "open captures/out.bmp");
    }
    //printf("dispdata: %s\n", scpio.c_str() );

    return 0;
}
