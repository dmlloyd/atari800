/* (C) 2000  Krzysztof Nikiel */
/* $Id: sound.h,v 1.1 2001/03/18 07:56:48 knik Exp $ */
#ifndef A800_SOUND_H
#define A800_SOUND_H

int initsound(int *argc, char *argv[]);
void uninitsound(void);
void sndrestore(void);
void sndhandler(void);

#endif
