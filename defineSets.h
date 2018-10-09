//
// Created by alfylinux on 29/07/18.
//
#ifndef CHAT_MULTILEVEL_DEFINESETS_H
#define CHAT_MULTILEVEL_DEFINESETS_H

#define firmwareVersion "0.04"
#define userDirName "Users"
#define chatDirName "Chats_ROOM"
#define serverConfFile "serverStat.conf"

#define chatTable "chatTableList.tab"
#define userTable "userTableList.tab"
#define chatConv "chatConversation.conv"



#define fflush(stdin) while(getchar()!='\n');

/** Define di setup delle funx **/
#define nAcceptTh 1
enum colorText {
	Titoli = 1, Comandi, ViewPan, StdoutPrint, ErrorPrint
};

#endif //CHAT_MULTILEVEL_DEFINESETS_H
