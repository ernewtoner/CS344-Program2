#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h> // For getpid()
#include <sys/stat.h>
#include <time.h>

// Simple Room struct that represents which rooms are connected by the corresponding integers in the global rooms array below
struct Room
{
  int numOutboundConnections;
  int outBoundConnections[6];
};

// Global array is Room 0 to Room 6 with connections 0 to 5
// Initialize all rooms to have no connections (-1)
// numOutBoundConnections is automatically initialized to 0 (http://c0x.coding-guidelines.com/6.7.8.html#1695
struct Room rooms[7] = { { .outBoundConnections = {-1, -1, -1, -1, -1, -1 } },
			 { .outBoundConnections = {-1, -1, -1, -1, -1, -1 } },
			 { .outBoundConnections = {-1, -1, -1, -1, -1, -1 } },
			 { .outBoundConnections = {-1, -1, -1, -1, -1, -1 } },
			 { .outBoundConnections = {-1, -1, -1, -1, -1, -1 } },
			 { .outBoundConnections = {-1, -1, -1, -1, -1, -1 } },
			 { .outBoundConnections = {-1, -1, -1, -1, -1, -1 } } };
struct Room* roomPtrs[7];

// Function prototypes
int GetRandomRoom();
bool CanAddConnectionFrom(int roomIndex);
bool ConnectionAlreadyExists(int x, int y);
void ConnectRoom(int x, int y);
bool IsSameRoom(int x, int y);

// Returns true if all rooms have 3 to 6 outbound connections, false otherwise
bool IsGraphFull()  
{
  int i;
  for(i=0; i < 7; i++) {
    // If any room has less than 3 connections, return false
    if (roomPtrs[i]->numOutboundConnections < 3)
      return false;
  }

  // Otherwise return true
  return true;
}

// Adds a random, valid outbound connection from a Room to another Room
void AddRandomConnection()  
{
  int randomRoom1, randomRoom2;
  
  while(true)
  {
    // Get a random room for the first room until one is chosen that a connection can be added to
    randomRoom1 = GetRandomRoom();

    if (CanAddConnectionFrom(randomRoom1) == true)
      break;
  }

  do
  {
    // Like above, also get a second random room until the conditions for successfully connecting are met
    // i.e. Room2 can have a connection added, is not the same room as Room1, and Room2 and Room1 don't already have a connection
    randomRoom2 = GetRandomRoom();
  }
  while(CanAddConnectionFrom(randomRoom2) == false || IsSameRoom(randomRoom1, randomRoom2) == true || ConnectionAlreadyExists(randomRoom1, randomRoom2) == true);

  // Connect Room1 to Room2 and vise versa
  ConnectRoom(randomRoom1, randomRoom2);
  ConnectRoom(randomRoom2, randomRoom1);
}

// Returns a random rooms[] index, does NOT validate if connection can be added
int GetRandomRoom()
{
  int randomIndex = rand() % 7; // Generate a random index between 0 and 6
  
  return randomIndex;
}

// Returns true if a connection can be added from Room x (< 6 outbound connections), false otherwise
bool CanAddConnectionFrom(int x) 
{
  if (roomPtrs[x]->numOutboundConnections < 6)
    return true;
  else
    return false;
}

// Returns true if a connection from Room number x to Room number y already exists, false otherwise
bool ConnectionAlreadyExists(int x, int y)
{ 
  int i;
  for(i=0; i < 6; i++) {
    if (roomPtrs[x]->outBoundConnections[i] == y)
      return true;
  }
  
  return false;
}

// Connects Rooms x and y together, does not check if this connection is valid
void ConnectRoom(int x, int y) 
{
  int i;
  // Iterate through all the outboundConnections of room number x
  for(i=0; i < 6; i++) {
    // Set the first free connection to y and increment x's number of outbound connections
    if (roomPtrs[x]->outBoundConnections[i] == -1) {
      roomPtrs[x]->outBoundConnections[i] = y;
      roomPtrs[x]->numOutboundConnections++;
      break;
    }
  }
}

// Returns true if Rooms x and y are the same Room, false otherwise
bool IsSameRoom(int x, int y) 
{
  return (x == y);
}

int GenerateRoomFiles() {
  int pid = getpid(); // Process ID
  char dirname[30]; // Directory Name
  char filepath[30]; // Full file path, dirname + filename

  // 10 hardcoded room names
  char* roomNames[] = {"Limbo", "Sanct", "Raven", "Badlands", "Gardens", "Mount", "Plains", "Road", "Karazen", "Forest"};

  // Create the string for the directory name with pid
  sprintf(dirname, "newtoner.rooms.%d/", pid);

  // Make directory and check if creation was successful
  int makeDirectory = mkdir(dirname, 0755);
  if (makeDirectory == -1) {
    perror("Error in GenerateRoomFiles(): ");
    return -1;
  } 
  
  // Shuffle the 10-element room name array randomly
  // Help from https://stackoverflow.com/questions/23176467/c-generate-random-numbers-without-repetition
  int i;
  for (i = 0; i < 10; i++) {
    int randomIndex = rand() % 10; // Generate a random index between 0 and 9

    // For extra randomization, we'll check if the random number is equal to our current index
    // Generate again if so
    if (randomIndex == i) {
      do {
	randomIndex = rand() % 10;
      }
      while (randomIndex == i);
    }

    // Swap current element at i with element at randomIndex
    char* temp = roomNames[i];
    
    roomNames[i] = roomNames[randomIndex];
    roomNames[randomIndex] = temp;
  }

  // Start processing individual files
  FILE *fp;

  // Generate random indexes from 0 to 6 (our 7 files used) for our start room and end room
  int randomStart = rand() % 7;
  int randomEnd = rand() % 7;

  // If both numbers are equal, continue picking random numbers until they aren't
  if (randomStart == randomEnd) {
    do {
      randomStart = rand() % 7;
      randomEnd = rand() % 7;
    }
    while (randomStart == randomEnd);
  }
  
  //// Iterate through files for each of the 7 rooms we'll have in the game
  for (i = 0; i < 7; i++) {
    // Concatenate the directory's name and the room's file name
    // e.g. newtoner.rooms.4202/ and Limbo
    strcpy(filepath, dirname);
    strcat(filepath, roomNames[i]);
    
    fp = fopen(filepath, "w");
    if ( fp == NULL ) {
      perror("Error in GenerateRoomFiles(): ");
      return -1;
    }

    // Associate the randomly chosen room name with a room built up in the graph
    fprintf(fp, "ROOM NAME: %s\n", roomNames[i]);

    // Iterate through the room's connections and write them to the output file
    int j;
    for (j = 0; j < 6; j++) {
      // If this is a valid connection, print the roomName that corresponds to the connected room's index, connection number = index + 1 to get rid of zero-indexing
      if (roomPtrs[i]->outBoundConnections[j] != -1)
	fprintf(fp, "CONNECTION %d: %s \n", j+1, roomNames[roomPtrs[i]->outBoundConnections[j]]);
      else
	// Otherwise it must be invalid (-1), break out of loop
	break;
    }

    // If i is our random start or end indexes, print the room type accordingly
    // Otherwise just make MID-ROOM
    if (i == randomStart) {
      fprintf(fp, "ROOM TYPE: START_ROOM\n");
    }
    else if (i == randomEnd) {
      fprintf(fp, "ROOM TYPE: END_ROOM\n");
    }
    else
      fprintf(fp, "ROOM TYPE: MID_ROOM\n");

    // Close every file
    fclose(fp);
  }

  return 0;
}

int main()
{
  // Initalize the roomPtrs array so we're not passing around the structs by value
  int i;
  for (i = 0; i < 7; i++) {
    roomPtrs[i] = &rooms[i];
  }
  
  // Seed the random number generator
  srand(time(NULL));

  // Create all connections in graph
  while (IsGraphFull() == false)
    {
      AddRandomConnection();
    }

  // Generate the room files
  GenerateRoomFiles();

  return 0;
}
