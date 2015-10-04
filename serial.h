typedef struct {

} serial_t;

typedef struct {

} queue_t;

typedef struct {

} crowd_t;

typedef struct {

} cond_t;

serial_t* Create_Serial();
Serial_Enter(serial_t*);
Serial_Exit(serial_t*);
queue_t* Create_Queue(serial_t*);
crowd_t* Create_Crowd(serial_t*);
int Queue_Empty(serial_t* serial, queue_t* queue);
int Crowd_Empty(serial_t* serial, crowd_t* crowd);
Serial_Enqueue(serial_t* serial, queue_t* queue, cond_t *func);
Serial_Join_Crowd(serial_t* serial, crowd_t* crowd, cond_t* func);
