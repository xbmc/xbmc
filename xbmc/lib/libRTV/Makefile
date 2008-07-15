OBJS=crypt.o guideclient.o GuideParser.o httpclient.o httpfsclient.o interface.o md5.o netclient.o rtv.o sleep.o

%.o : %.c
	gcc -c $< -D_LINUX -D__unix__
%.o : %.cpp
	g++ -c $< -D_LINUX

librtv-i486-linux.a : $(OBJS)
	ar rs $@  $(OBJS)

clean:
	$(RM) $(OBJS) librtv-i486-linux.a
