#ifndef PLAYER_MESSAGE_H
#define PLAYER_MESSAGE_H

#define MESSAGE_MAX 4

#define CTRL_CMD_RESPONSE   (0xffff)

typedef enum {
    CMD_EXIT            = (1 << 0),
    CMD_PLAY            = (1 << 1),
    CMD_PLAY_START      = (1 << 2),
    CMD_STOP            = (1 << 3),
    CMD_START           = (1 << 4),
    CMD_NEXT            = (1 << 5),
    CMD_PREV            = (1 << 6),
    CMD_PAUSE           = (1 << 7),
    CMD_RESUME          = (1 << 8),
    CMD_SEARCH          = (1 << 9),
    CMD_FF              = (1 << 10),
    CMD_FB              = (1 << 11),
    CMD_SWITCH_AID      = (1 << 12),
    CMD_SWITCH_SID      = (1 << 13),    
    CMD_CTRL_MAX        = (1 << 31),
} ctrl_cmd_t;

typedef enum {
    CMD_LOOP            = (1 << 0),
    CMD_NOLOOP          = (1 << 1),
    CMD_BLACKOUT        = (1 << 2),
    CMD_NOBLACK         = (1 << 3),
    CMD_NOAUDIO         = (1 << 4),
    CMD_NOVIDEO         = (1 << 5),
    CMD_MUTE            = (1 << 6),
    CMD_UNMUTE          = (1 << 7),
    CMD_SET_VOLUME      = (1 << 8),
    CMD_SPECTRUM_SWITCH = (1 << 9),
    CMD_SET_BALANCE     = (1 << 10),
    CMD_SWAP_LR         = (1 << 11),
    CMD_LEFT_MONO       = (1 << 12),
    CMD_RIGHT_MONO      = (1 << 13),
    CMD_SET_STEREO      = (1 << 14),   
    CMD_EN_AUTOBUF      = (1 << 15),
    CMD_SET_AUTOBUF_LEV = (1 << 16),
    CMD_MODE_MAX        = (1 << 31),
} ctrl_mode_t;

typedef enum {
    CMD_GET_VOLUME     = (1 << 0),
    CMD_GET_VOL_RANGE  = (1 << 1),
    CMD_GET_PLAY_STA   = (1 << 2),
    CMD_GET_CURTIME    = (1 << 3),
    CMD_GET_DURATION   = (1 << 4),
    CMD_GET_MEDIA_INFO = (1 << 5),
    CMD_LIST_PID       = (1 << 6),
    CMD_GET_MAX        = (1 << 31),
} get_info_t;

typedef struct {
    float min;
    float max;
} volume_range_t;

typedef struct {
    ctrl_cmd_t ctrl_cmd;
    get_info_t info_cmd;
    ctrl_mode_t set_mode;
    int pid;
    int cid;
    union {
        char *filename;
        char *file_list;
        int param;
        float f_param;
    };
    union {
        int param1;
        float f_param1;
    };
    union {
        int param2;
        float f_param2;
    };
} player_cmd_t;

int message_free(player_cmd_t * cmd);
player_cmd_t * message_alloc(void);
int cmd2str(player_cmd_t *cmd, char *buf);

#endif

