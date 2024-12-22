
#include <i_net.h>
#include <m_argv.h>
#include <doomstat.h>
#include <stdlib.h>

void I_InitNetwork (void)
{
    boolean		trueval = true;
    int			i;
    int			p;
	
    doomcom = (doomcom_t*)malloc (sizeof (*doomcom) );
    memset (doomcom, 0, sizeof(*doomcom) );
    
    // set up for network
    i = M_CheckParm ("-dup");
    	
	if (i && i< myargc-1)
    {
		doomcom->ticdup = myargv[i+1][0]-'0';
		if (doomcom->ticdup < 1)
	    	doomcom->ticdup = 1;
		if (doomcom->ticdup > 9)
	    	doomcom->ticdup = 9;
    }
    else
		doomcom-> ticdup = 1;
	
    if (M_CheckParm ("-extratic"))
		doomcom-> extratics = 1;
    else
		doomcom-> extratics = 0;
		
    //p = M_CheckParm ("-port");
    //if (p && p<myargc-1)
    //{
	//	DOOMPORT = atoi (myargv[p+1]);
	//	printf ("using alternate port %i\n",DOOMPORT);
    //}
    
    // parse network game options,
    //  -net <consoleplayer> <host> <host> ...
    i = M_CheckParm ("-net");
    if (!i)
    {
		printf("Initializing single player game\n");
		// single player game
		netgame = false;
		doomcom->id = DOOMCOM_ID;
		doomcom->numplayers = doomcom->numnodes = 1;
		doomcom->deathmatch = false;
		doomcom->consoleplayer = 0;
		return;
    }

    //netsend = PacketSend;
    //netget = PacketGet;
    //netgame = true;

    // parse player number and host list
    //doomcom->consoleplayer = myargv[i+1][0]-'1';

    //doomcom->numnodes = 1;	// this node for sure
	
    //i++;
/* 
    while (++i < myargc && myargv[i][0] != '-')
    {
		sendaddress[doomcom->numnodes].sin_family = AF_INET;
		sendaddress[doomcom->numnodes].sin_port = htons(DOOMPORT);
		if (myargv[i][0] == '.')
		{
	    	sendaddress[doomcom->numnodes].sin_addr.s_addr = inet_addr (myargv[i]+1);
		}
		else
		{
	    	hostentry = gethostbyname (myargv[i]);
	    	if (!hostentry)
				I_Error ("gethostbyname: couldn't find %s", myargv[i]);
	    
			sendaddress[doomcom->numnodes].sin_addr.s_addr = *(int *)hostentry->h_addr_list[0];
		}
	
		doomcom->numnodes++;
    }
	
    doomcom->id = DOOMCOM_ID;
    doomcom->numplayers = doomcom->numnodes;
    
    // build message to receive
    insocket = UDPsocket ();
    BindToLocalPort (insocket,htons(DOOMPORT));
    ioctl (insocket, FIONBIO, &trueval);

    sendsocket = UDPsocket ();
*/
}

void I_NetCmd (void)
{
	printf("I_NetCmd\n");
}

