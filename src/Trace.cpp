#include <stdio.h>
#include <fstream>
#include <iostream>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

// Code taken from 
// https://github.com/embecosm/esp1-systemc-tlm/blob/master/sysc-models/simple-soc/TermSC.h

#include "Trace.h"

void Trace::xtermLaunch( char *slaveName ) {
    char *arg;
    char *fin = &(slaveName[strlen( slaveName ) - 2]);
    
    if( NULL == strchr(fin, '/' )) {
        arg = new char[2 + 1 + 1 + 20 + 1];
        sprintf( arg, "-S%c%c%d", fin[0], fin[1], ptMaster );
    } else {
        char *slaveBase = ::basename( slaveName );
        arg = new char[2 + strlen( slaveBase ) + 1 + 20 + 1];
        sprintf( arg, "-S%s/%d", slaveBase, ptMaster );
    }
    
    char *argv[3];
    argv[0] = (char *)( "xterm" );
    argv[1] = arg;
    argv[2] = NULL;
    
    execvp( "xterm", argv );
}

void Trace::xtermKill( const char *mess ) {
    
    
    if( -1 != ptSlave ) {			// Close down the slave
        
        close( ptSlave );			// Close the FD
        ptSlave  = -1;
    }
    
    if( -1 != ptMaster ) {		// Close down the master
        close( ptMaster );
        ptMaster = -1;
    }
    
    if( xtermPid > 0 ) {			// Kill the terminal
        kill( xtermPid, SIGKILL );
        waitpid( xtermPid, NULL, 0 );
    }
    
    if( NULL != mess ) {			// If we really want a message
        perror( mess );
    }
    
    
}

void Trace::xtermSetup(void) {
    ptMaster = open("/dev/ptmx", O_RDWR);
    grantpt( ptMaster );
    
    unlockpt( ptMaster );
    
    char *ptSlaveName = ptsname( ptMaster );
    ptSlave = open( ptSlaveName, O_RDWR );	// In and out are the same
    
    struct termios  termInfo;
    tcgetattr( ptSlave, &termInfo );
    
    termInfo.c_lflag &= ~ECHO;
    termInfo.c_lflag &= ~ICANON;
    tcsetattr( ptSlave, TCSADRAIN, &termInfo );
    
    xtermPid = fork();
    
    if (xtermPid == 0) {
        xtermLaunch( ptSlaveName );
    }
}

SC_HAS_PROCESS(Trace);
Trace::Trace(sc_module_name name): sc_module(name)
,socket("socket") {
    
    socket.register_b_transport(this, &Trace::b_transport);
    
    xtermSetup();
}

Trace::~Trace() {
    xtermKill( NULL );
}
void Trace::b_transport( tlm::tlm_generic_payload& trans, sc_time& delay ) {
    //tlm::tlm_command cmd = trans.get_command();
    //sc_dt::uint64    adr = trans.get_address() / 4;
    unsigned char*   ptr = trans.get_data_ptr();
    //unsigned int     len = trans.get_data_length();
    //unsigned char*   byt = trans.get_byte_enable_ptr();
    //unsigned int     wid = trans.get_streaming_width();
    
    write(ptSlave, ptr, 1);    
    
    trans.set_response_status( tlm::TLM_OK_RESPONSE );
}
