#include <inc/lib.h>
#define N 12
#define HALT_SIG 0xDEADDEAD

int A[N][N];
//matrix present data . 
envid_t  proc[N+1][N+1];
//matrix present processes' env_id.
int stat[N+1][N+1];
//matrix present processes' opeartion.
/*
c : controller 
e : emitter ( for the vector I ) 
r : receptor ( for the vector AI ) 
w : worker ( recevie data , hold matrix and , calc the sum and send it to other workers ) 

the processes are arranged like a matrix .
if N is 4 , then the matrix will looks like :  
	  | 0 1 2 3 4
	--+---------- 
	0 | c e e e e 
	1 | r w w w w 
	2 | r w w w w 
	3 | r w w w w 
	4 | r w w w w 

 */

int procx , procy ; 

void initmatrix(){
	int i , j ; 
	for ( i = 0 ; i < N ; i ++ )
		for ( j = 0 ; j < N ; j ++ ) 
			A[i][j]  =  1 ; 
	proc[0][0] = sys_getenvid(); 
	stat[0][0] = 0 ; 
	// set the controller 

	for ( i = 1 ; i < N + 1 ; i ++ ) 
		stat[0][i] = 1 ; 
	// set emitter ; 

	for ( i = 1 ; i < N + 1 ; i ++ ) 
		stat[i][0] = 2 ; 
	// set receptor ; 
		
	for ( i = 1 ; i < N + 1 ; i ++ ) 	
		for ( j = 1 ; j < N + 1 ; j ++ ) 
			stat[i][j] = 3 ; 
	// set worker ; 

}

void emitter()
{
	envid_t s1 , w1 ; 
	int data ;
	while ( 1 ) { 
		data = ipc_recv(&s1,0,0) ; 
		if ( ( s1 == proc[0][0] ) && ( data == HALT_SIG ) ) { 
			cprintf(" [ Emiiter %d ] ",procy-1 ) ; 
			cprintf("Now is halting. ");
			exit();
		}
		cprintf(" [ Emitter %d ] ",procy-1 ) ; 
		cprintf("Received data %d from Controller.Send it to Worker.\n",data);
		ipc_send( proc[procx+1][procy] , data , 0 , 0 ) ; 
	}		
}

void worker()
{
	//Initilize the Worker : the Controller give the Rightside Process's ID to it.
	envid_t init_id = -1 ; 
	envid_t right_id = -1 ; 
	if ( procy != N ) {
		while ( init_id != proc[0][0] ) {
			right_id = ipc_recv( &init_id , 0 , 0 ) ; 
			if ( right_id == HALT_SIG ) {
				cprintf(" [ Worker(%d,%d) ] Now is halting.\n", procx , procy ) ;
				exit();	
			}
		}
	}
	envid_t s1 , w1 , s2 , w2 ;
	s2 = -1 ;  
	int d1 , d2 ;
	d2 = d1 = 0 ;
	while ( true ) { 
		d1 = ipc_recv(&s1,0,0);
		if ( procy != N ) d2 = ipc_recv(&s2,0,0);
		if ( ( s1 == proc[0][0] ) && ( d1 == HALT_SIG) ) {
			cprintf(" [ Worker(%d,%d) ] Now is halting.\n", procx , procy ) ;
			exit();
		}
		if ( ( s2 == proc[0][0] ) && ( d2 == HALT_SIG) ) {
			cprintf(" [ Worker(%d,%d) ] Now is halting.\n", procx , procy ) ;
			exit();
		}
		cprintf(" [ Worker(%d,%d) ] Received data , will calc  and sent it to woker.\n" , procx , procy );
		if ( s1 == right_id )  {
			envid_t tmp = s2 ; s2 = s1 ; s1 = tmp ; 
			int tmp2 = d2 ; d2 = d1 ; d1 = tmp2; 
		}
		int sum = d2 + d1 * A[procx-1][procy-1];
		int ele = d1 ; 
		ipc_send(proc[procx][procy-1],sum,0,0);
		if ( procx != N ) 
			ipc_send(proc[procx+1][procy],ele,0,0);
	}	 
}

void receptor()
{
	envid_t s1 , w1 ; 
	int data ;
	while ( true ) { 
		data = ipc_recv(&s1,0,0) ; 
		if ( ( s1 == proc[0][0] ) && ( data == HALT_SIG ) ) { 
			cprintf(" [ Receptor %d ] ", procx-1 ) ; 
			cprintf("Now is halting. ");
			exit();
		}
		cprintf(" [ Receptor %d ] ",procx-1 ) ; 
		cprintf("Received data %d from Work.Send it to Controller.\n",data);
		ipc_send( proc[0][0] , data , 0 , 0 ) ; 
	}
}

void controller_terminate()
{
	int i , j ; 
	for ( i = 0 ; i <= N ; i ++ ) 
		for ( j = 0 ; j <= N ; j ++ ) { 
			if ( ( i + j ) == 0 ) continue ; 
			ipc_send( proc[i][j] , HALT_SIG , 0 , 0 ) ; 
		} 
	exit();			
}


void
umain(int argc, char **argv)
{
	// Prepare Step 1 : initial the basic data struct .	
	initmatrix();

	// Prepare 2 : fork the processes in a specific way , 
	//             and store the information into the 'proc' matrix 
	// 	       use the global value 'procx' and 'procy' to direct the position.
	procx = N ; procy = 0 ; 
	while (1) {
		envid_t nowptr = -1 ; 
		while ( nowptr < 0 ) {
			nowptr = fork() ; 
			if ( nowptr == 0 ) {
				if ( stat[procx][procy] == 1 ) {
					emitter();
				}			
				if ( stat[procx][procy] == 2 ) {
					receptor();
				}
				if ( stat[procx][procy] == 3 ) {
					worker();
				}
				exit();
			} 
		}
		proc[procx][procy] = nowptr ; 
		if ( ( procx == 0 ) && ( procy == N ) ) break ; 
		if ( procy == N ) {
			procx = procx - 1 ; 
			procy = 0 ;                    // if meet the last element in the row , then move to the next row. 

			if ( procx == 0 ) procy = 1 ;  // ignore the controller ;
		} else {
			procy ++; 
		}
	}
	
	// Prepare Step 3 : only the controller can reach here.
	// 	            now send the corresponding rightside process's id to each worker . 
	int i , j , k ; 	 

	for ( i = 1 ; i <= N ; i ++ ) 
		for ( j = 1 ; j <= N-1 ; j ++ ) {
			ipc_send( proc[i][j] , proc[i][j+1] , 0 , 0 ) ;
		}
	
	// Having prepared for all the things we need , we will run the big procedure on processes.
	// In this simple case ,  we only perform one iteration , but actually for any times interation it would be correct and safe . 
 
	// Run Step 1 : set the Ans vector to all zeros . 			
	int Ans[N+1];
	memset( Ans , 0 , sizeof(Ans) ) ;

	// Run Step 2 : dispatch data to emitter .
	for ( i = 1 ; i <= N ; i ++ ) {
		cprintf(" [ Controller ] Dispatching the data to Emitter(%d) @Process(%d).\n",i,proc[0][i]);
		ipc_send( proc[0][i] , i , 0 , 0 ) ;
	}	

	// Run Step 3 : receiving data from receptor .
	for ( i = 1 ; i <= N ; i ++ ) {
		envid_t recp_id ; 
		result = ipc_recv( &recp_id , 0 , 0 ) ;    
		for ( j = 1 ; j <= N ; j ++ ) {
			if ( recp_id == proc[j][0] ) {
				cprintf(" [ Contorller ] ");
				cprintf("Received the data %d from Receptor(%d) @Process(%d).\n",result , j,proc[j][0]);
				Ans[ j - 1 ]  = result ; break ; 
			}
		}
		if ( j == N + 1 ) { 
			cprintf(" [ Error in Contorller ] Fail in receiving , all the processes will be halt.\n");
			controller_terminate();
		}
	}
	}
	// Run Step 4 : show the 'Ans' matrix and terminate the whole big procedure. 
	cprintf(" [ Final Answer ] \n") ;
	for ( i = 0 ; i < N ; i ++ ) 
		cprintf(", %8d",Ans[i]);
	cprintf("\n [ Final Answer ] \n");
	controller_terminate();
}





