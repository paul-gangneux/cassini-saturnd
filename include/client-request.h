#ifndef CLIENT_REQUEST_H
#define CLIENT_REQUEST_H

#include <stdint.h>
#include "timing.h"
#include "custom-string.h"

#define CLIENT_REQUEST_LIST_TASKS 0x4c53              // 'LS'
#define CLIENT_REQUEST_CREATE_TASK 0x4352             // 'CR'
#define CLIENT_REQUEST_REMOVE_TASK 0x524d             // 'RM'
#define CLIENT_REQUEST_GET_TIMES_AND_EXITCODES 0x5458 // 'TX'
#define CLIENT_REQUEST_TERMINATE 0x544d               // 'TM'
#define CLIENT_REQUEST_GET_STDOUT 0x534f              // 'SO'
#define CLIENT_REQUEST_GET_STDERR 0x5345              // 'SE'

typedef struct {
	uint32_t ARGC;
	string_p* ARGVs;
} commandline;

typedef struct {
	uint16_t OPCODE;
} cli_request_simple;

typedef struct {
	uint16_t OPCODE;
    uint64_t TASKID;
} cli_request_task;

typedef struct {
	uint16_t OPCODE;
    timing TIMING;
    commandline COMMANDLINE;
} cli_request_create;

typedef struct {
	char opcode[2];
    char min[8];
	char hours[4];
	char day[1];
    char argc[4];
} cli_request_create_chars;


void commandline_free(commandline* cmdl) {
	for (uint32_t i = 0; i < cmdl->ARGC; i++) {
		string_free(cmdl->ARGVs[i]);
	}
	free(cmdl->ARGVs);
	free(cmdl);
}


#endif // CLIENT_REQUEST_H
