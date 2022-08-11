# include "fiber/fiber_pool.h"
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
        while(true) {
            int input_size = read(STDIN_FILENO, &s[read_cnt], sizeof(s)-read_cnt);
            while (input_size > 0) {
                if (s[read_cnt] == '\n') break;
                read_cnt++;
                input_size--;
            }
            if(s[read_cnt] == '\n') break;
        }
        s[++read_cnt] = '\0';
        write(STDOUT_FILENO, s, read_cnt);
    }, 0);

    f->Join();
    fp.Stop();
}

