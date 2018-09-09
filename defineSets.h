//
// Created by alfylinux on 29/07/18.
//
#ifndef CHAT_MULTILEVEL_DEFINESETS_H
#define CHAT_MULTILEVEL_DEFINESETS_H

#define firmwareVersion "0.02"
#define userDirName "Users"

#define fflush(stdin) while(getchar()!='\n');

/** Define di setup delle funx **/
#define nAcceptTh 2
enum colorText {
	Titoli = 1, Comandi, ViewPan, StdoutPrint, ErrorPrint
};

#endif //CHAT_MULTILEVEL_DEFINESETS_H
