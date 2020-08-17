#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<fcntl.h>
#include<stdio.h>
#include<string.h>
#include<sys/time.h>
#include<unistd.h>
#include<stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdbool.h>//bool用
#define BUFMAX 40
#define PORT_NO (u_short)20000
#define Err(x) {fprintf(stderr,"-"); perror(x); exit(0);}
////////////////////////////////////////////////////////////////////////////////
struct Place{
  int line;
  int row;
  int borw; //黒＝-1、白=１、何もない=０//server = black, client = white
};
///////////////////////////////////////////////////////////////////////////////////
static int number,sofd,nsofd,retval;
static struct hostent *shost;
static struct sockaddr_in own,svaddr;
static char buf[BUFMAX], shostn[BUFMAX];
static fd_set mask; 
static struct timeval tm; 
static int putstone = 0;

void servpro();
void cliepro();
void placedecode(char *place, int *x, int *y);//文字から数字に変換する関数
int winserv(struct Place place[]);//サーバが買ったときの判定をする関数
int wincli(struct Place place[]);//クライアントが買ったときの判定をする関数

////////////////////////////////////////////////////////////////////////////////////
unsigned long MyColor(Display *display, char *color)
{
  Colormap cmap;
  XColor c0, c1;
  cmap = DefaultColormap(display, 0);
  XAllocNamedColor(display, cmap, color, &c1, &c0);
  return (c1.pixel);
}//色を使うためのもの、配布資料にある。

///////////////////////////////////////////////////////////////////////////////////////
main(){
  
  while( 1 ) {
    printf( "server:1 client:2\n" );
    scanf( "%d", &number );
    printf( "\n" );
    if( number == 1 || number == 2 ) break;
  }
  
  if( number == 1 ) servpro();
  if( number == 2 ) cliepro();
}
/////////////////////////////////////////////////////////////////////////////////////////
void servpro(int argc, char **argv){

  tm.tv_sec = 0; 
  tm.tv_usec = 1; 
  struct Place place[42];//盤の一つ一つに構造体を置く

  int i, j;

  for(i = 0; i < 42; i++){
    place[i].line = i/7;
    place[i].row = i%7;
    place[i].borw = 0;
  }//初期化

 _Bool serorcli = true; //bool型を使って置く人を判定する

 ////////////////////////////////////////////////////////
  
  if( gethostname( shostn, sizeof( shostn ) ) < 0 )
    Err( "gethostname" );
 
  printf( "hostname is %s", shostn );
  printf( "\n" );
  
  shost = gethostbyname( shostn );
  if( shost == NULL ) Err( "gethostbyname" );
 
  bzero( (char *)&own, sizeof( own ) );
  own.sin_family = AF_INET;
  own.sin_port = htons(PORT_NO);
  bcopy( (char *)shost->h_addr, (char *)&own.sin_addr, shost->h_length );
 
  sofd = socket( AF_INET, SOCK_STREAM, 0 );
  if( sofd < 0 ) Err( "socket" );
  if( bind( sofd, (struct sockaddr*)&own, sizeof( own ) ) < 0 ) Err( "bind" );

  listen( sofd, 1 );
  
  nsofd = accept( sofd, NULL, NULL );
  if( nsofd < 0 ) Err( "accept" );
  close( sofd );

  write( 1, "BATTLE_START\n",14);

  ////////////////////////////////////////////////////
  
  Display *d;//Display構造体
  Window w;//Window構造体
  XEvent event;//Event共有体
  GC gc1, gc2, gc3, gc4;//GCのID用の変数を用意
  XGCValues gv1;
  XTextProperty w_name;
  unsigned long MyColor();//色用
  unsigned long black, white, board;//黒の石、白の石、盤の色
  d=XOpenDisplay (NULL);//ｘサーバとの接続：ローカルXサーバ
  white = MyColor(d, "white");//白
  black = MyColor(d, "black");//黒
  board = MyColor(d, "DarkGray");//盤にぴったりな色
  w=XCreateSimpleWindow (d, RootWindow(d,0), 20, 20, 500, 500, 2,
    WhitePixel(d,DefaultScreen(d)), BlackPixel(d,DefaultScreen(d)));//Windowの作成
  w_name.value = (unsigned char*)"Connect Four! =server=";
  w_name.encoding = XA_STRING;
  w_name.format = 8;
  w_name.nitems = strlen("Connect Four! =server=");
  XSetWMProperties(d, w, &w_name, NULL,argv, argc, NULL, NULL, NULL);
  XSelectInput(d,w,ExposureMask|ButtonPressMask|KeyPressMask);
  gv1.line_width = 2;
  gc1 = XCreateGC (d, w, GCLineWidth, &gv1);//線を書く用
  gc2 = XCreateGC(d, w, 0, 0);//黒い石、白い石の枠線用,mojiyou
  gc3 = XCreateGC(d, w, 0, 0);//白い石用
  gc4 = XCreateGC(d, w, 0, 0);//盤用
  XSetForeground(d, gc1, white);//
  XSetForeground(d, gc2, black);//黒
  XSetForeground(d, gc3, white);//白
  XSetForeground(d, gc4, board);//盤
  XMapWindow(d,w);//ウィンドウのバッファリング
  ////////////////////////////////////////////////////////
  
  while( 1 ) {
    
    FD_ZERO( &mask );
    FD_SET( nsofd, &mask );
    
    retval = select( nsofd+1, &mask, NULL, NULL, &tm ); //変化があったらselect関数で教えてくれる
   
    if( retval < 0 ) Err( "select" ); 
    
    if( FD_ISSET( nsofd, &mask ) ) { //maskのnsofdが変化していた場合
      
      read( nsofd, buf, BUFMAX );//相手からのメッセージをbufに読み込む
      printf("		client : %s\n",buf);
      if(strncmp(buf, "ERROR",5) == 0){ //ERRORメッセージを受け取った時 
	close(nsofd);
	exit(0);
      }
      else if(strncmp(buf, "YOU-WIN",7) == 0){//YOU-WINのメッセージを受け取った時
	close(nsofd);
	exit(0);
      
      }	
      else if(strncmp(buf, "PLACE-", 6) == 0){//PLACEのメッセージを受け取った時
        putstone = putstone+1;
	int x, y;
	//client is white
        placedecode(buf, &x, &y);
	i = 0;
	for(i = 0; i<42;i++){
		if(place[i].line == y && place[i].row == x){
			place[i].borw = 1;
			XFillArc(d, w, gc3, (place[i].row+1)*50+10, (place[i].line+1)*50+10, 30, 30, 0*64, 360*64);
                        XDrawArc(d, w, gc2, (place[i].row+1)*50+10, (place[i].line+1)*50+10, 30, 30, 0*64, 360*64);
		}
	}
	//相手の石：白の描画
	if (wincli(place) == 1) {//相手が勝った場合
	        write(nsofd, "YOU-WIN", BUFMAX);
	        printf("YOU-LOSE\n");
	        close(nsofd);
		exit(0);
	}

        if(putstone == 42){//引き分けになった場合
        	printf("DRAW FILLED\n");
        	write(nsofd, "ERROR",BUFMAX);
		close(nsofd); 
		exit(0);
        }
        serorcli = !serorcli;//打つ人交換
      }
      bzero(buf,BUFMAX);
    }
    
    /////////////////////////////////////////////////////////
    
    if (XPending(d) != 0) {//今回のプログラムで一番重要な部分
      int x, y;
      XNextEvent(d, &event);
      switch(event.type){
	case Expose:
        	XFillRectangle(d, w, gc4, 50, 50, 50*7, 50*6);//盤の作成
        	int l;
        	for(l = 1; l<= 8; l++){
                	XDrawLine(d, w, gc1, l*50, 50, l*50, 7*50);
        	}//線を引く
	    	for(l =1; l<=7;l++){
			XDrawLine(d, w, gc1, 50, l*50, 8*50, l*50);
		}//線を引く
            	for(i = 0; i <42; i++){
                	if(place[i].borw == -1){
                    		XFillArc(d, w, gc2, (place[i].row+1)*50+10, (place[i].line+1)*50+10, 30, 30, 0*64, 360*64);//黒の石の再描画//
                	}//黒の石
                	if(place[i].borw == 1){
                    		XFillArc(d, w, gc3, (place[i].row+1)*50+10, (place[i].line+1)*50+10, 30, 30, 0*64, 360*64);
                    		XDrawArc(d, w, gc2, (place[i].row+1)*50+10, (place[i].line+1)*50+10, 30, 30, 0*64, 360*64);
                	}//白の石
            	}//石の再描画
            
            	break;
 
        case ButtonPress:
		if( event.xbutton.x < 50 | event.xbutton.y < 50 |  event.xbutton.x > 50*16 | event.xbutton.y > 50*16)
                	break;//盤の外をクリックしても反応しない
            	int signal = 0;
            	if(serorcli == true){//自分の番の時sever is black
			x = (event.xbutton.x / 50) - 1;
            		for(i=41; i>=0; i--){
                		if(place[i].row == x && place[i].borw == 0){
                        		place[i].borw = -1; 
					XFillArc(d, w, gc2, (place[i].row+1)*50+10, (place[i].line+1)*50+10, 30, 30, 0*64, 360*64);//黒の石の再描画//
		          		putstone++;
                          		serorcli=!serorcli;
                          		char result_x[3];
                          		char result_y[3];
                    			char result[9];
                    			sprintf(result_x,"%2X",place[i].row);
                    			sprintf(result_y,"%2X",place[i].line);
                          		strcpy(result,"PLACE-XY");
                          		result[6]=result_x[1];
                          		result[7]=result_y[1];
                          		write(nsofd, result, BUFMAX);
					printf("you : PLACE-%d%d\n", place[i].row , place[i].line);
					i = -1;
				}
			}
		}else{//相手の番の時
                	printf("ERROR: not your turn.\n");
            	} 
            	break;
	}//switch
    	}//If Pending
  	}//While
  	close(nsofd);
}//serve func

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cliepro(int argc, char**argv){
  tm.tv_sec = 0; 
  tm.tv_usec = 1; 
  struct Place place[42];
  int i;
  for(i = 0; i < 42; i++){
    place[i].line = i/7;
    place[i].row = i%7;
    place[i].borw = 0;
  }//初期化
  _Bool serorcli = false; 
  //////////////////////////////////////////////////
  puts("hostname?");
  scanf("%s", shostn);
  
  shost = gethostbyname(shostn);
  if (shost == NULL) {
    Err("gethostbyname");
  }
  
  bzero((char *)&svaddr, sizeof(svaddr));
  svaddr.sin_family = AF_INET;
  svaddr.sin_port = htons(PORT_NO);
  bcopy((char *)shost->h_addr_list[0], (char *)&svaddr.sin_addr, shost->h_length);
  
  sofd = socket(AF_INET, SOCK_STREAM, 0);
  if (sofd < 0) {
    Err("socket");
  }
  
  connect(sofd, (struct sockaddr *)&svaddr, sizeof(svaddr));
  write( 1, "BUTTLE START\n", 14 );

  ///////////////////////////////////////////////////////////
  
  Display *d;//Display構造体
  Window w;//Window構造体
  XEvent event;//Event共有体
  GC gc1, gc2, gc3, gc4;//GCのID用の変数を用意
  XGCValues gv1;
  XTextProperty w_name;
  unsigned long MyColor();//色用
  unsigned long black, white, board;//黒の石、白の石、盤の色
  d=XOpenDisplay (NULL);//ｘサーバとの接続：ローカルXサーバ
  white = MyColor(d, "white");//白
  black = MyColor(d, "black");//黒
  board = MyColor(d, "DarkGray");//盤にぴったりな色
  w=XCreateSimpleWindow (d, RootWindow(d,0), 20, 20, 500, 500, 2,
		 WhitePixel(d,DefaultScreen(d)), BlackPixel(d,DefaultScreen(d)));//Windowの作成
  w_name.value = (unsigned char*)"Connect Four! =client=";
  w_name.encoding = XA_STRING;
  w_name.format = 8;
  w_name.nitems = strlen("Connect Four! =client=");
  XSetWMProperties(d, w, &w_name, NULL,argv, argc, NULL, NULL, NULL);
  XSelectInput(d,w,ExposureMask|ButtonPressMask|KeyPressMask);
  gv1.line_width = 2;
  gc1 = XCreateGC (d, w, GCLineWidth, &gv1);//線を書く用
  gc2 = XCreateGC(d, w, 0, 0);//黒い石、白い石の枠線用,mojiyou
  gc3 = XCreateGC(d, w, 0, 0);//白い石用
  gc4 = XCreateGC(d, w, 0, 0);//盤用
  XSetForeground(d, gc1, white);//
  XSetForeground(d, gc2, black);//黒
  XSetForeground(d, gc3, white);//白
  XSetForeground(d, gc4, board);//盤
  XMapWindow(d,w);//ウィンドウのバッファリング
  ///////////////////////////////////////////////////////////////
  
  while( 1 ) {
        
    FD_ZERO( &mask ); 
    FD_SET( sofd, &mask );  
   
    retval = select( sofd+1, &mask, NULL, NULL, &tm );  
    if( retval < 0 ) Err( "select" ); 
    
    if( FD_ISSET( sofd, &mask ) ) { 
            
      	read( sofd, buf, BUFMAX );  
      	printf("		server : %s\n",buf);
      	if(strncmp(buf, "ERROR",5) == 0){  
	        close(sofd);
	        exit(0);
      	}
      	else if(strncmp(buf, "YOU-WIN",7) == 0){
	      close(sofd);
	      exit(0);
      	} 
      	else if(strncmp(buf, "PLACE-", 6) == 0){
		putstone = putstone+1;
		int x, y;
	      	placedecode(buf, &x, &y);
	      	i = 0;
        	for(i = 0; i<42;i++){//server is black, client is white
                	if(place[i].line == y && place[i].row == x){
                        	place[i].borw = -1;
                        	XFillArc(d, w, gc2, (place[i].row+1)*50+10, (place[i].line+1)*50+10, 30, 30, 0*64, 360*64);
                        	
                	}
		        //oponent black stone
			if (winserv(place) == 1) {//相手が勝った場合
                		write(sofd, "YOU-WIN", BUFMAX);
                		printf("YOU-LOSE\n");
                		close(sofd);
                		exit(0);
        		}
	      		if(putstone == 42){
	        		printf("DRAW FIlled\n");
	          		write(sofd, "ERROR",BUFMAX);
	          		close(sofd);
	          		exit(0);
	      		}
		}
        	serorcli = !serorcli;
	}
      bzero(buf,BUFMAX); 
    }

        /////////////////////////////////////////////////////////

    if (XPending(d) != 0){
        XNextEvent(d, &event);
        switch(event.type){
            case Expose:
	              	XFillRectangle(d, w, gc4, 50, 50, 50*7, 50*6);//盤の作成
		      	int l;
        		for(l = 1; l<= 8; l++){
                		XDrawLine(d, w, gc1, l*50, 50, l*50, 7*50);
        		}//線を引く
	    		for(l =1; l<=7;l++){
				XDrawLine(d, w, gc1, 50, l*50, 8*50, l*50);
			}//線を引く
	              	for(i = 0; i <42; i++){
	                	if(place[i].borw == -1){
	                     		XFillArc (d, w, gc2, (place[i].row+1)*50+10, (place[i].line+1)*50+10, 30, 30, 0*64, 360*64);//黒の石の再描画
	                	}
	                	if(place[i].borw == 1){
	                     		XFillArc (d, w, gc3, (place[i].row+1)*50+10,(place[i].line+1)*50+10, 30, 30, 0*64, 360*64);
	                     		XDrawArc(d, w, gc2, (place[i].row+1)*50+10, (place[i].line+1)*50+10, 30, 30, 0*64, 360*64);
	                	}
	              	}
	                
	                break;
	
                case ButtonPress:
	                if( event.xbutton.x < 50 | event.xbutton.y < 50 |  event.xbutton.x > 50*16 | event.xbutton.y > 50*16)
	                    break;//盤の外をクリックしても反応しない
	                int x, y;
			int signal = 0;
			if(serorcli == true){//自分の番の時sever is black
				x = (event.xbutton.x / 50) - 1;
            			for(i=41; i>=0; i--){
                			if(place[i].row == x && place[i].borw == 0){
                        			place[i].borw = 1;
						XFillArc (d, w, gc3, (place[i].row+1)*50+10,(place[i].line+1)*50+10, 30, 30, 0*64, 360*64);
		                        	XDrawArc(d, w, gc2, (place[i].row+1)*50+10, (place[i].line+1)*50+10, 30, 30, 0*64, 360*64);
						putstone++;
                          			serorcli=!serorcli;;
                          			char result_x[3];
                          			char result_y[3];
	                    			char result[9];
        	            			sprintf(result_x,"%2X",place[i].row);
                	    			sprintf(result_y,"%2X",place[i].line);
                        	  		strcpy(result,"PLACE-XY");
                          			result[6]=result_x[1];
                          			result[7]=result_y[1];
	                          		write(sofd, result, BUFMAX);
						printf("you : PLACE-%d%d\n", place[i].row , place[i].line);

						i = -1;
					}
				}
			}else{
	                    printf("ERROR: not your turn.\n");
	                }    
	                break;
	}//switch
    }//xpending 
  }//main loop
  close(sofd);
}//client func

/////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

void placedecode(char *place, int *x, int *y)//文字から整数に変換
{
  *x = (place[6] >= 'A') ? ((place[6] - 'A') + 10) : (place[6] - '0');
  *y = (place[7] >= 'A') ? ((place[7] - 'A') + 10) : (place[7] - '0');
}

int wincli(struct Place place[])//クライアントの勝ち負け判定
{
	int i;
	int j;
	for(i = 0; i<=5; i++){
		for(j=0; j<4;j++){
			if((place[j+7*i].borw == 1) &&(place[j+7*i+1].borw == 1)&&(place[j+7*i+2].borw == 1)&&(place[j+7*i+3].borw == 1)){
				return 1;
			}					

		} 
	}
	for(i =0;i<=20;i++){
		if((place[i].borw == 1) && (place[i+7].borw == 1)&&(place[i+7*2].borw == 1)&&(place[i+7*3].borw == 1)){
			return 1;
		}
	}
	for(i = 0;i<=2; i++){
		for(j = 3; j<=6;j++){
			if((place[j+i*7].borw == 1) && (place[j+i*7+6].borw == 1)&&(place[j+i*7+6*2].borw == 1)&&(place[j+i*7+6*3].borw == 1)){
				return 1;
			}
		}
	}
	for(i = 0;i<=2;i++){
		for(j=0; j<=3;j++){
			if((place[j+i*7].borw == 1) && (place[j+i*7+8].borw == 1) && (place[j+i*7+8*2].borw == 1) &&(place[j+i*7+8*3].borw == 1) ){
				return 1;
			}
		}
	}
	return 0;
}

int winserv(struct Place place[])//サーバの勝ち負け判定
{
	int i;
       	int j;
        for(i = 0; i<=5; i++){
                for(j=0; j<4;j++){
                        if((place[j+7*i].borw == -1) &&(place[j+7*i+1].borw == -1)&&(place[j+7*i+2].borw == -1)&&(place[j+7*i+3].borw == -1)){
                                return 1;
                        }

                }
        }
        for(i =0;i<=20;i++){
                if((place[i].borw == -1) && (place[i+7].borw == -1)&&(place[i+7*2].borw == -1)&&(place[i+7*3].borw == -1)){
                        return 1;
                }
        }
        for(i = 0;i<=2; i++){
                for(j = 3; j<=6;j++){
                        if((place[j+i*7].borw == -1) && (place[j+i*7+6].borw == -1)&&(place[j+i*7+6*2].borw == -1)&&(place[j+i*7+6*3].borw == -1)){
                                return 1;
                        }
                }
        }
        for(i = 0;i<=2;i++){
                for(j=0; j<=3;j++){
                        if((place[j+i*7].borw == -1) && (place[j+i*7+8].borw == -1) && (place[j+i*7+8*2].borw == -1) &&(place[j+i*7+8*3].borw == -1) ){
                                return 1;
                        }
                }
        }
        return 0;

}

