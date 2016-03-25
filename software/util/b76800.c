
#include <asm/termios.h>
#include <fcntl.h>
#include <err.h>
#include <linux/serial.h>

#include<stdio.h>
#include <errno.h>

#include <string.h>
#include <unistd.h>

#include <signal.h>
#include <sys/select.h>

int serial_fd;

volatile int exit_flag = 0;

void sig_handler(int signum, siginfo_t *info, void *ptr)
{
   exit_flag = 1;
}

void catch_term()
{
   static struct sigaction sa;

   memset(&sa, 0, sizeof(sa));

   sa.sa_sigaction = sig_handler;
   sa.sa_flags = SA_SIGINFO;

   sigaction(SIGTERM, &sa, NULL);
}

int serial_open(const char *device, int rate)
{
   int fd;
   struct termios2 tio;

   if ((fd = open(device,O_RDWR|O_NOCTTY)) == -1)
      return -1;


   ioctl(fd, TCGETS2, &tio);


   tio.c_cflag &= ~CBAUD;
   tio.c_cflag |= BOTHER;
   tio.c_ispeed = rate;
   tio.c_ospeed = rate;



   ioctl(fd, TCSETS2, &tio);
   return fd;
}


void read_stdin_to_serial()
{
   char buf[1024];
   int r=read(STDIN_FILENO,buf,1024);
   if(r==-1)
   {
      exit_flag=1;
      return;
   }

   if(r>0)
      write(serial_fd,buf,r);

}

void read_serial_to_stdout()
{
   char buf[1024];
   int r=read(serial_fd,buf,1024);
   if(r==-1)
   {
      exit_flag=1;
      return;
   }

   if(r>0)
      write(STDOUT_FILENO,buf,r);
}


void io_loop(void)
{
   fd_set fds;
   int maxfd;
   struct timeval tv;

   tv.tv_usec=0;
   tv.tv_sec=5;

   maxfd = (serial_fd > STDIN_FILENO)?serial_fd:STDIN_FILENO;

   while(!exit_flag)
   {

      FD_ZERO(&fds);
      FD_SET(serial_fd, &fds);
      FD_SET(STDIN_FILENO, &fds);

      select(maxfd+1, &fds, NULL, NULL, &tv);

      if (FD_ISSET(STDIN_FILENO, &fds))
      {
         read_stdin_to_serial();
      }

      if (FD_ISSET(serial_fd, &fds))
      {
         read_serial_to_stdout();
      }


   }
}


int main (int argc, char *argv[])
{

   if(argc<3)
   {
      fprintf(stderr,"args: serial_device baudrate\n");
      return 1;
   }

   serial_fd = serial_open(argv[1],atoi(argv[2]));

   catch_term();

   fcntl(STDIN_FILENO, F_SETFL, 0); //make sure stdin blocks

   io_loop();


   return 0;
}


