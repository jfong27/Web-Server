#include "httpd.h"
#include "simple_net.h"

int main(int argc, char *argv[]){
   in_port_t port = strtol(argv[1], NULL, 10);
   int fd, new_fd;

   if((fd = create_service(port)) < 0){
      fprintf(stderr,"Error opening socket\n");
      exit(-1);
   }



   while(1){
      if((new_fd = accept_connection(fd)) < 0){
         fprintf(stderr,"Error opening connection\n");
         exit(1);
      }

      handle_request(new_fd);
      close(new_fd);
   }

}


void handle_request(int fd){
   pid_t pid;
   char *req = NULL;
   char *reply = NULL;

   if((pid = fork()) < 0){
      perror("Fork");
      exit(1);
   }
   else if(pid == 0){
      req = read_request(fd);
      reply = proc_req(req);

      if(reply[0] != 'H'){
         reply = err_response(reply);
      }

      send(fd, reply, strlen(reply)+1, 0);
      exit(1);
   }
   else{
      /*waitpid(pid, NULL, 0);
       * if I wait for pid, only one
       * request is handled at a time
       * if i don't wait, multiple
       * requests are handled at the same
       * time */
   }

}


char *read_request(int fd){
   int size = START_SIZE;
   char *buf = (char*)checked_malloc(size);
   char *start = buf;
   int pos = 0;

   while(read(fd,buf++,1) > 0){
      if(start[pos++] == '\n')
         break;
      if(pos >= size){
         size += 50;
         start = realloc(start, size);
         buf = start + pos;
      }
   }

   return start;
}

char *proc_req(char *req){

   char *type = strtok(req," ");
   char *file = strtok(NULL, " ");
   char *file_copy = strdup(file);
   char *cgi_check = strtok(file_copy,"/");
   char *reply = NULL;
   char *cont = NULL;
   char *perm = NULL;
   char *temp = checked_malloc(strlen(file)+2);

   strcpy(temp, ".");
   strcat(temp, file);
   file = temp;


   if(strcmp(cgi_check, "cgi-like") == 0)
      return cgi(file); 
   if(strstr(file, "/..") != 0)
      return err_response("403");


   if(strcmp(type, "GET")==0){
      reply = get_header(file);
      if(reply[0] != 'H')
         return reply;

      if((perm = check_perm(file)) != NULL)
         return perm;

      cont = get_contents(file);

      strcat(reply,cont);
   }
   else if(strcmp(type, "HEAD")==0){
      reply = get_header(file);
   }
   else{
      reply = err_response("501");
      exit(1);
   }

   free(file_copy);
   return reply;

}

/*bad malloc */

char *get_header(char *file){
   struct stat fs;
   char *header = checked_malloc(500);
   char len[10];


   if(access(file, F_OK) == -1)
      return "404";
   stat(file, &fs);

   sprintf(len, "%d", (int)fs.st_size);
    
   strcpy(header, "HTTP/1.0 200 OK\r\n");
   strcat(header, "Content-Type: text/html\r\n");
   strcat(header, "Content-Length: ");
   strcat(header, len);
   strcat(header, "\r\n\r\n");


   return header;


}

/**********************************************
 * Function: check_perm
 *    Checks for the existence of the file and 
 *    if it is readable. 
 * Returns: 
 *    String of the error number, or NULL upon
 *    success; 
 **********************************************/

char *check_perm(char *file){
   if(access(file, F_OK) == -1){
      return "404";
   }
   if(access(file, R_OK) == -1){
      return "403";
   }

   return NULL;
}


char *get_contents(char *file){
   FILE *fp;
   char *contents;
   char *file_temp;
   long file_size;

   file_temp = checked_malloc(strlen(file)+3);
   strcpy(file_temp, "./");
   strcat(file_temp, file);
   
   fp = fopen(file_temp, "r");
   fseek(fp, 0, SEEK_END);
   file_size = ftell(fp);
   fseek(fp,0, SEEK_SET);

   
   contents = checked_malloc(file_size + 1);
   fread(contents, file_size, 1, fp);

   fclose(fp);
   free(file_temp);
   contents[file_size] = 0;

   return contents;

}

/*bad malloc size*/
char *cgi(char *file){
   int i = 0;
   char **command = checked_malloc(15);
   char *response;
   strtok(file, "/");
   strtok(NULL, "/");
   command[0] = add_dot_slash(strtok(NULL, "?/"));


   while(command[i] != NULL){
      command[++i] =  strtok(NULL, "&");
   }

   response = exec_cgi(command);

   free(command);

   return response;
}


char *exec_cgi(char **cmd){
   pid_t pid; 
   int out;
   char *pid_int = checked_malloc(10);
   char *header;
   char *cont;

   pid = fork();
   if(pid < 0){
      return err_response("500");
   }
   else if(pid == 0){
      sprintf(pid_int, "%d", getpid());
      out = open(pid_int, O_CREAT | O_RDWR, S_IRUSR 
               | S_IWUSR);
      chdir("cgi-like");
      printf("%s\n", cmd[0]+2);
      dup2(out, 1);
      execvp(cmd[0]+2, cmd);
      printf("cmd: %s\n", cmd[0]);
      if(check_perm(cmd[1]) != NULL){
         printf("ASDADS\n");
         return check_perm(cmd[0]);
      }
      execvp(cmd[0], cmd);
      return err_response("500");
   }
   else{
      waitpid(pid, NULL, 0);
      sprintf(pid_int, "%d", pid);
      pid_int = add_dot_slash(pid_int);
      header = get_header(pid_int);
      cont = get_contents(pid_int);
      strcat(header, cont);
      remove(pid_int);

   }

   return header;

}

char *add_dot_slash(char *file){
   char *new = checked_malloc(strlen(file) + 2);
   strcpy(new, "./");
   strcat(new, file);

   return new;
}


char *err_response(char *type){
   char *error_msg;
   char *resp = checked_malloc(50);
   int err = strtol(type, NULL, 10);


   strcpy(resp, "HTTP/1.0 ");

   switch(err){
      case 400: 
         strcat(resp, "400 Bad Request");
         error_msg = get_contents("Err_400.html");
         break;
      case 403:
         strcat(resp, "403 Permission Denied");
         error_msg = get_contents("Err_403.html");
         break;
      case 404: 
         strcat(resp, "404 Not Found");
         error_msg = get_contents("Err_404.html");
         break;
      case 500: 
         strcat(resp, "Internal Error");
         error_msg = get_contents("Err_500.html");
         break;
      case 501:
         strcat(resp, "Not Implemented");
         error_msg = get_contents("Err_501.html");
         break;
   }


   strcat(resp, "\r\nContent-Type: text/html\r\n");
   strcat(resp,error_msg);


   return resp; 


}


void *checked_malloc(size_t size){
   void *spot = malloc(size);
   if(spot == NULL){
     perror("Malloc");
     exit(1);
   }
  return spot; 
}
