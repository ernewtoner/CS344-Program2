#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

// Room struct storing the Room's individual attributes and strings that correspond to the Room's connections 
struct Room
{
  char name[100];
  int numOutboundConnections;
  char* outBoundConnections[6];
  char roomType[100];
};

// Global rooms array, we'll access the rooms by their index, Room 0 to Room 6
struct Room rooms[7];
struct Room* roomPtrs[7];

// Create a mutex
pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;

// Find the most recent newtoner.rooms.* directory and copy to string argument
// Lots of help from the lectures for this
void GetMostRecentDir(char* newestDirName)
{
  int newestDirTime = -1;
  char targetDirPrefix[32] = "newtoner.rooms."; // Prefix we're searching for

  // Pointer and structures to contain the directory information
  DIR* dp;
  struct dirent *fileInDir;
  struct stat entryAttributes;
  
  // Open up the current directory
  dp = opendir(".");

  // If the open was successful
  if (dp > 0) {
    // While we're still reading entries
    while ((fileInDir = readdir(dp)) != NULL) {
      // If the entry has the prefix, stat the entry
      if (strstr(fileInDir->d_name, targetDirPrefix) != NULL) { 
	stat(fileInDir->d_name, &entryAttributes); 	        

	// If it is a directory, check if its time is greater than our known newest time
	if (S_ISDIR(entryAttributes.st_mode)) {
	  if ((int)entryAttributes.st_mtime > newestDirTime) {
	    // If so, update our newest time with the new directory name
	    newestDirTime = (int)entryAttributes.st_mtime;
	    strcpy(newestDirName, fileInDir->d_name);
	  }
	}
      }
    }
  }

  closedir(dp);
}

// Get the current time and write it to a file in the most recent rooms directory
void* getTime(void* newestDirName) {
  // Infinite loop that runs only when this thread is unblocked
  while (1) { 
    // Lock the mutex so only this thread is running
    // Will be blocked until the mutex is unlocked by the time input in Prompt()
    pthread_mutex_lock(&myMutex);
    usleep(100);

    // Cast the void* parameter to a char*
    char* newestDirNamePtr = (char*)newestDirName;
    
    FILE* fp;
    char filepath[30]; // Full file path, dirname + filename
    
    sprintf(filepath, "%s/currentTime.txt", newestDirNamePtr);

    fp = fopen(filepath, "w");
    if ( fp == NULL ) {
      perror("Error in getTime(): ");
    }

    // Help from https://www.geeksforgeeks.org/strftime-function-in-c/ for time syntax
    time_t t;
    struct tm *tmp;
    char currentTime[50];
    time(&t);
    
    tmp = localtime(&t);

    // Format time as follows: 1:03pm, Tuesday, September 13, 2016
    strftime(currentTime, sizeof(currentTime), "%l:%M%P, %A, %B %d, %Y", tmp);
  
    // Write time to currentTime.txt in the newest rooms.* directory
    fprintf(fp, "\n %s\n\n", currentTime);

    // Close the file
    fclose(fp);
    
    // Unlock the mutex again for the main thread to take over
    pthread_mutex_unlock(&myMutex);
    usleep(100);
    }
  
  return NULL;
}

// Takes the current room the player is located in, i.e. roomPtrs[currentRoomIndex]
void Prompt(int currentRoomIndex, char* newestDirName) {
  char input[24]; // User's input

  // Number of steps the player took and their path to victory, we want to retain these values until the end
  static int numSteps = 0;
  static char pathToVictory[256] = "";
  
  // If this is the end room, exit game
   if (strcmp(roomPtrs[currentRoomIndex]->roomType, "END_ROOM") == 0) {
    printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n%s", numSteps, pathToVictory);
   
    exit(0);
  }

  // Print the current prompt
  printf("CURRENT LOCATION: %s\n", roomPtrs[currentRoomIndex]->name);
  printf("POSSIBLE CONNECTIONS: ");
  
  int numRoomConnections = roomPtrs[currentRoomIndex]->numOutboundConnections;

  // Print connections of current room
  // Iterate through the connections, printing a comma afterwards for all but the last entry
  int i;
  for (i = 0; i < numRoomConnections; i++) {
    if (i == (numRoomConnections - 1))
      printf("%s.\n", roomPtrs[currentRoomIndex]->outBoundConnections[i]);
    else
      printf("%s, ", roomPtrs[currentRoomIndex]->outBoundConnections[i]);
  }

  printf("WHERE TO? >");
  scanf("%s", input);

  // Store the global rooms array index of the room entered (if a room is entered)
  int roomInputIndex = -1;
  
  ///// Check for 'time' input first
  while (strcmp(input, "time") == 0) {
    
    // Have the main thread unlock the mutex for the secondary thread to lock
    pthread_mutex_unlock(&myMutex);

    // Wait 100 microseconds and lock again
    usleep(100);
    pthread_mutex_lock(&myMutex);

    // Read from the currentTime.txt file written by the second thread
    // Help from https://stackoverflow.com/questions/14002954/c-programming-how-to-read-the-whole-file-contents-into-a-buffer for reading files
    FILE* fp;
    char filepath[64]; // Full file path, dirname + filename
    
    sprintf(filepath, "%s/currentTime.txt", newestDirName);

    fp = fopen(filepath, "r");
    if ( fp == NULL ) {
      perror("Error in Prompt(): ");
    }
    
    // Move file pointer to end of file
    fseek(fp, 0, SEEK_END);

    // Size of the buffer is the current position of the file pointer
    int filesize = ftell(fp);

    // Move file pointer back to the beginning
    fseek(fp, 0, SEEK_SET);

    // Read in file into a string buffer
    char* buffer = calloc(1, filesize + 1);
    fread(buffer, filesize, 1, fp);

    // Print buffer to the player
    printf("%s", buffer);

    // Close file and free buffer
    fclose(fp);
    free(buffer);

    printf("WHERE TO? >");
    scanf("%s", input);
    }
  ///// End time input
  
  // Otherwise iterate through the current room's connections
  for (i = 0; i < numRoomConnections; i++) {
    // Attempt to find a connection's room name that matches the input string
    if (strcmp(roomPtrs[currentRoomIndex]->outBoundConnections[i], input) == 0) {      
      // Get the room's index in the global rooms array if it's a valid connection
      int j;
      for (j = 0; j < 7; j++) {
	if (strcmp(input, roomPtrs[j]->name) == 0)
	  roomInputIndex = j;
      }

      // And concatenate the input to the path to victory
      strcat(pathToVictory, input);
      strcat(pathToVictory, "\n");
    }
  }

  // If the input string didn't match a room name in the roomPtrs array, output error
  if (roomInputIndex == -1) {
    printf("\nHUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
    
    // Run the Prompt again with the current room index
    Prompt(currentRoomIndex, newestDirName);
  }
  // Otherwise print newline, increment the number of steps, and call Prompt again with the inputted room's index
  else {
    printf("\n");
    
    numSteps++;
    Prompt(roomInputIndex, newestDirName);
    }
}

int main(void)
{  
  // Have the main thread lock the mutex
  pthread_mutex_lock(&myMutex);

  // Get the most recent directory and store in newestDirName
  char newestDirName[512];
  GetMostRecentDir(newestDirName);

  // Initalize the roomPtrs array so we're not passing around the structs by value
  int i;
  for (i = 0; i < 7; i++) {
    roomPtrs[i] = &rooms[i];
  }
  
  // Open the found newest directory
  DIR* dp;
  struct dirent *fileInDir;
  dp = opendir(newestDirName);
  
  if (dp == NULL) {
    perror("Error in main() :");
    return -1;
  }
  
  char* filename; // Name of the file without directory path
  char filepath[56]; // Directory path + name of file
  int currentRoomIndex = 0; // Room we're currently processing in the global rooms array
  int startRoomIndex; // The start room's index in the global array
  
  // Read in files in the directory
  while ((fileInDir = readdir(dp)) != NULL) {
    filename = fileInDir->d_name;

    // Skip over . and .. entries in the current directory and currentTime.txt if it exists
    if ((strcmp(filename, ".") == 0) || (strcmp(filename, "..") == 0) || (strcmp(filename, "currentTime.txt") == 0))
      continue;
    else {
      // Concatenate the directory name with / and filename to create the filepath
      strcpy(filepath, newestDirName);
      strcat(filepath, "/");
      strcat(filepath, filename);
    }

    /*
      Read in information from room files formatted like so:
      ROOM NAME: XYZZY
      CONNECTION 1: PLOVER
      CONNECTION 2: Dungeon
      CONNECTION 3: twisty
      ROOM TYPE: START_ROOM

      Help from https://www.geeksforgeeks.org/readwrite-structure-file-c/
    */    
    FILE *fp;
    fp = fopen(filepath, "r");

    if ( fp == NULL ) {
      perror("Error in main(): ");
      return -1;
    }

    char buf[100]; // Space for the value we're after, i.e. 'RoomName'
    char head1[100]; // First header space, i.e. 'CONNECTION'
    char head2[100]; // Second header space, i.e. '1:'

    // Get room name and copy to Room struct in the global array
    fscanf(fp, "%*s %*s %s", buf);
    strcpy(rooms[currentRoomIndex].name, buf);
        
    // Keep track of how many connections have been added
    // Pointer used to make the below while loop more readable
    rooms[currentRoomIndex].numOutboundConnections = 0;
    int* numConnections = &rooms[currentRoomIndex].numOutboundConnections;
    
    // Scan file as long as we're getting formatted date
    while (fscanf(fp, "%s %s %s", head1, head2, buf) == 3) {
      // Get connection room names until we we hit the ROOM TYPE: line
      if (strcmp(head1, "CONNECTION") == 0) {
	
	// Allocate a string buffer in the Room that stores the room name
	// Start at the first index in the Room's outBoundConnections array and keep incrementing
	rooms[currentRoomIndex].outBoundConnections[(*numConnections)] = calloc(1, sizeof(char) * 101);
	strcpy(rooms[currentRoomIndex].outBoundConnections[(*numConnections)], buf);

	// Increment the Room's number of connections
	(*numConnections)++;
      }
      else if (strcmp(head1, "ROOM") == 0) {
	strcpy(rooms[currentRoomIndex].roomType, buf);
	
	// If this is the start room, keep track of the index since we'll begin the Prompt here
	if (strcmp(buf, "START_ROOM") == 0) {
	  startRoomIndex = currentRoomIndex;
	}
      }
    }
    
    fclose(fp);
    currentRoomIndex++;
  }

  // Close the directory
  closedir(dp);

  // Create secondary thread for the getTime() function, won't execute until the main thread unlocks the mutex
  pthread_t threadID;
  int resultInt;
  resultInt = pthread_create(&threadID, NULL, getTime, (void*)newestDirName);
  
  assert(resultInt == 0);

  // Begin the prompt at the start Room's index and pass in the latest directory found
  Prompt(startRoomIndex, newestDirName);

  // Clean up dynamically allocated string array, outBoundConnections
  int m, n;
  for (m=0; m < 7; m++) {
    for (n=0; n < rooms[m].numOutboundConnections; n++) {
      free(rooms[m].outBoundConnections[n]);
    }
  }

  pthread_mutex_destroy(&myMutex);
  
  return 0;
}
