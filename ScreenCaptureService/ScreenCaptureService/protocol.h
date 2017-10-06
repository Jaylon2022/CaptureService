#ifndef _H_PROTOCOL
#define _H_PROTOCOL

enum {
    MSG_MODE_ACK = 1,

    MSG_MODE_MOUSE_SET,
    MSG_MODE_MOUSE_MOVE,
    MSG_MODE_MOUSE_HIDE,

    MSG_MODE_STREAM_CREATE = 10,
    MSG_MODE_STREAM_DATA,
    MSG_MODE_STREAM_DESTROY,

    MSG_END
};

/* head */
typedef struct protocol_head_s {
    int type;
    int length;
} protocol_head_t;

/* data */
typedef struct protocol_msg_data_s {
    int type;
    int data[0];
} protocol_msg_data_t;

/* MSG_MOUSE_SET */
typedef struct msg_mode_ack {
    int ok;
} msg_mode_ack;

typedef struct msg_mouse_set {
    int width;
    int height;
    int hot_spot_x;
    int hot_spot_y;
    char data[0];
} msg_mouse_set;

/* MSG_MOUSE_MOVE */
typedef struct msg_mouse_move {
    int x;
    int y;
} msg_mouse_move;

/* MSG_MOUSE_HIDE */
typedef struct msg_mouse_hide {
    int hide;
} msg_mouse_hide;

#define ALIGN_BITS(x, align) (((x)+ (align - 1))&(~(align - 1)))

/* MSG_STREAM_CREATE */
typedef struct msg_stream_create {
    int stream_id;      /* ignore */
    int stream_width;
    int stream_height;
} msg_stream_create;


/* MSG_STREAM_DESTROY */
typedef struct msg_stream_destroy {
    int stream_id;      /* ignore */
} msg_stream_destroy;

void socket_send_data(int type, char *data, int size);

#endif /* _H_PROTOCOL */
