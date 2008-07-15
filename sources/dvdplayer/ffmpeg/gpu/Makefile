NAME := gpu_player
SRC := gpu_player.c
PATHS := -I"../libavcodec" \
	-I"../libavformat" \
	-I"../libavutil" \
	-I".." \
	-L"../libavdevice" \
	-L"../libavformat" \
	-L"../libavcodec" \
	-L"../libavutil"  
CFLAGS := -g3 -O0
LIBS :=  -lz -lm -ldl -ldl -L/usr/lib -lSDL -lGL -lGLEW -lglut \
	-lavformat -lavcodec -lavdevice -lavutil  -lbz2
#FILE :=../saikano-ova-h264-blocking-01.mkv  
FILE := ../Ganges.E01.1080p.L4.1.mkv

default: compile run

compile:
	cd ..; make; cd gpu;
	gcc $(SRC) $(CFLAGS) $(PATHS) $(LIBS) -o $(NAME)

run:
	@./$(NAME) $(FILE) 5 motion
proto:
	gcc gpu_mo_proto.c gpu_utils.c $(CFLAGS) $(PATHS) $(LIBS) -o gpu_proto
	@./gpu_proto

scratch:
	gcc gpu_utils.c $(CFLAGS) $(PATHS)  -c -o gpu_utils.o
	gcc glm.c $(CFLAGS) $(PATHS) -c -o glm.o
	gcc gpu_scratch.c gpu_utils.o glm.o $(CFLAGS) $(PATHS) $(LIBS) -o scratch_app
	./scratch_app 
