

/***********************************************
 * Function: dirwalk
 *    Opens directory, walks through it, creating
 *    a list. Then calls function to sort the
 *    list before calling a function to go 
 *    through the list.
 ***********************************************/

void dirwalk(char *dir){
   struct dirent *dp;
   DIR *dfd;
   File *head;
   File *curr;

   head = (File*)checked_malloc(sizeof(File));
   head->title = NULL;
   head->next = NULL;
   curr = head;

   if ((dfd = opendir(dir)) == NULL) {
      fprintf(stderr, "Cannot open directory: %s\n",
               dir);
      exit(1);
   }

   while ((dp = readdir(dfd)) != NULL) {
      if (strcmp(dp->d_name, ".") == 0
            || strcmp(dp->d_name, "..") == 0)
         continue;
      else {
         if(head->title == NULL){
            curr->title = strdup(dp->d_name);
            continue;
         }
         curr->next = (File*)checked_malloc(sizeof(File));
         curr = curr->next;
         curr->title = strdup(dp->d_name);
         curr->next = NULL;
      }
   }
   closedir(dfd);
   trav_nodes(head, dir, f);
     
      
}

/***********************************************
 * Function: trav_nodes
 *    Travels through the list, constructs 
 *    pathname, calls function to process the 
 *    file and call actions. Frees the list 
 *    while it goes through it. 
 ***********************************************/

void trav_nodes(File *head, char *dir, struct Flags f){
   char *name = NULL;
   File *curr;
   while(head != NULL && head->title != NULL){
      name = (char*)checked_malloc(strlen(dir)+strlen(head->title)+2);
      strcpy(name, dir);
      strcat(name, "/");
      strcat(name, head->title);

      process_file(f, name, head->title);
           
      curr = head;
      head = head->next;
      if(curr->title != NULL)
         free(curr->title);
      free(curr);
   }
   free(head);

}

/***********************************************
 * Function: process_file
 *    Checks the file for name switch and if it
 *    satisfies the switch. If it is a directory,
 *    it will call the dirwalk function with it.
 ***********************************************/

void process_file( struct Flags f, char *name, char *sub){
   struct stat fs;
   stat(name, &fs);

   action(name, f);
   if(S_ISDIR(fs.st_mode)){
      dirwalk(name );
   }
   free(name);
}

/***********************************************
 * Function: action
 *    Carries out either print or exec. Command
 *    needs to be duplicated and sliced in order
 *    to be accepted by execvp. 
 ***********************************************/

void action(char *dir, struct Flags f){

   char *temp;
   char *temp2;
   char *found;
   char **command = (char**)checked_malloc((f.cmd_len+1) 
                                      *sizeof(char*)+8);
   int i = 0;
   int j;
   if(f.action == PRINT){
      printf("%s\n", dir);
   }
   else if(f.action == EXEC){
      temp = strdup(f.cmd);
      temp2 = temp;
      while((found = strsep(&temp,"\n ")) != '\0'){
         if(*found == '\0')
            break;
         if(strcmp(found, "{}")==0){
            command[i++] = strdup(dir);
         }
         else{
            command[i++] = strdup(found);
         }
      }
      command[i] = NULL;
      free(temp2);
      new_process(command);
      for(j = 0; j < i; j++)
        free(command[j]); 
   }
   else{
      fprintf(stderr,"Usage: ./sfind filename");
      fprintf(stderr,"[-name str] -print | exec cmd \\;\n");
      exit(-1);
   }
   free(command);

}

/***********************************************
 * Function: new_process
 *    Forks a child process, which execs the 
 *    provided command.
 ***********************************************/

void new_process(char **command){
   pid_t pid;

   pid = fork();
   if(pid < 0){
      perror("Fork failed");
   }
   else if(pid == 0){
      execvp(command[0], command);
      perror("Exec failed");
      exit(1);
   }
   else{
      waitpid(pid, NULL, 0);
   }
}


