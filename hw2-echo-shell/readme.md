michaelgrantwarshowsky@Michaels-MacBook-Pro hw2-echo-shell % ./client -p "Hello there server, how are you?"
HEADER VALUES 
  Proto Type:    PROTO_CS_FUN
  Proto Ver:     VERSION_1
  Command:       CMD_PING_PONG
  Direction:     DIR_RECV
  Term:          TERM_FALL 
  Course:        NONE
  Pkt Len:       35

RECV FROM SERVER -> PONG: Hello there serv
michaelgrantwarshowsky@Michaels-MacBook-Pro hw2-echo-shell % ./client
HEADER VALUES 
  Proto Type:    PROTO_CS_FUN
  Proto Ver:     VERSION_1
  Command:       CMD_CLASS_INFO
  Direction:     DIR_RECV
  Term:          TERM_FALL 
  Course:        CS472
  Pkt Len:       35

RECV FROM SERVER -> CS472: Welcome to computer networks
michaelgrantwarshowsky@Michaels-MacBook-Pro hw2-echo-shell % ./client -c cs577
HEADER VALUES 
  Proto Type:    PROTO_CS_FUN
  Proto Ver:     VERSION_1
  Command:       CMD_CLASS_INFO
  Direction:     DIR_RECV
  Term:          TERM_FALL 
  Course:        cs577
  Pkt Len:       41

RECV FROM SERVER -> cs577: Software architecture is important