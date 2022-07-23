# include "fiberpool.h"
# include "logger.h"
# include <fcntl.h>

# include <stdio.h>

using namespace MyRPC;

#define NUM_THREADS 1


int main()
{
    FiberPool fp(NUM_THREADS);
    char s[100];

    memset(s, 0, sizeof(s));
    fp.Start();

    auto f = fp.Run([&s](){
        int read_cnt = 0;
        int input_size = read(STDIN_FILENO, s, sizeof(s));
        while(input_size> 0){
            if(s[read_cnt]=='\n') break;
            read_cnt++;
        }
        s[++read_cnt] = '\0';
        write(STDOUT_FILENO, s, read_cnt);
    }, 0, false);

    f.Join();
    fp.Stop();
}

