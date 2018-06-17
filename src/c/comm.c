#include <pebble.h>
#include "comm.h"

//New type Snapshot to be sent to server
typedef struct Snapshot {
  time_t timestamp;
  unsigned short steps;
  unsigned short causeOfSnapshot;
} Snapshot;

#define MAX_SNAPSHOT_RAM 30 //Max number of snapshot that we can hold in one array of 256 bytes in persistent memory

//int MAX_SNAPSHOT_INDEX = 4; //(MAX_SNAPSHOT_RAM -1) //= 191; TO BE REMOVED USING MAX SNAPSHOT RAM ONLY
//MAX_NUMBER_OF_SNAPSHOTS 7 //Current number of snapshots in memory stacked cause not sent via Bluetooth --> careful it takes memory --> move to persistent memory when full 

static Snapshot snapshots[MAX_SNAPSHOT_RAM]; //holds current snapshots
static Snapshot sbuffer[MAX_SNAPSHOT_RAM]; //buffer to read the snapshots in persistent memory

int current_snapshotIndex = -1;
int persistent_snapshotIndex = -1;
int mostRecentSnapshot = 1;
int finalStepCount = 0; //JESSICA not sure if needed - cause if enough spcae 
int arrayNumber = 0; //Number of persistent storage arrays we have written into

//Flags: Whether last message sent was from the current snapshot list or persistent storage
bool current_snapshot_sent = false;  
bool persistent_snapshot_sent = false; 



//Creates a message and writes to the dictionary for sending over bluetooth
static void message(Snapshot snapArray[], int counter) {
	
  //Shout not happen. If there is a message to be sent, snapshot index is increased by 1.
  if(counter < 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Tried to send message with an empty queue");
    return;
  }

   //Query the bluetooth connection service for the current Pebble app connection status.
  if(!connection_service_peek_pebble_app_connection()) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Bluetooth connection unavailable.");
    return;
  }

    DictionaryIterator *out;
	
	//What would be the problem at the beginning of the message?
	if(app_message_outbox_begin(&out) == APP_MSG_OK) {
	  
	//Put the snapshot at snapshotIndex into a temporary snapshot
    Snapshot snapshot = snapArray[counter];
    time_t timestamp = snapshot.timestamp;
    int steps = snapshot.steps;
	if (steps>finalStepCount) finalStepCount = snapshot.steps;
    //int experimentCondition = snapshot.experimentCondition;
    int causeOfSnapshot = (int)snapshot.causeOfSnapshot;
    APP_LOG(APP_LOG_LEVEL_INFO, "Cause of snapshot: %d", causeOfSnapshot);
		
	
    //What is this function??? Is it to send to the database to MostRecentCallHome?
    dict_write_int(out, AppKeyMostRecentSnapshot, &mostRecentSnapshot, sizeof(int), true);
    //most recent snapshot is now back to 0
	if(mostRecentSnapshot == 1){
      mostRecentSnapshot = 0;
    }
    
	//Writes timestamp, steps, cause of snapshot, con
    dict_write_int(out, AppKeyTimestamp, &timestamp, sizeof(int), true);
    dict_write_int(out, AppKeySteps, &steps, sizeof(int), true);
    dict_write_int(out, AppKeyFinalSteps, &finalStepCount, sizeof(int), true);
    dict_write_int(out, AppKeyCauseOfSnapshot, &causeOfSnapshot, sizeof(int), true);
	//IS THIS STILL SENT TO THE SERVER EVEN THOUGH snapshot.experimentCondition is commented out?
    dict_write_int(out, AppKeyExperimentConditionSentForServer, &experimentConditionThatUserIsIn, sizeof(int), true);
    dict_write_int(out, AppKeyUsersStepGoal, &usersStepGoal, sizeof(int), true);
	
	  if(app_message_outbox_send() == APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Sending message worked");
	  current_snapshot_sent = true; 
    }
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error beginning message");
	}
}

//Save current snapshotArray to persistent storage (can be optimized by not moving full array - e.g., when app closes)
void saveToPersistentStorage(){
	
  //Number of array in persistent memory increases
  arrayNumber++;

  // Save current snapshotArray to persistent storage
  // NEATER OPTION IS TO MAKE A FILE FIFO BUT CAN INCUR TIME PROBLEMS
  switch(arrayNumber){
	  case 1:
		    persist_delete(PersistKeySnapshotArray1);
	  		persist_write_data(PersistKeySnapshotArray1, snapshots, sizeof(snapshots));
			return;
	  case 2:
			persist_delete(PersistKeySnapshotArray2);
			persist_write_data(PersistKeySnapshotArray2, snapshots, sizeof(snapshots));
			return;		   
	  case 3: 
			persist_delete(PersistKeySnapshotArray3);
			persist_write_data(PersistKeySnapshotArray3, snapshots, sizeof(snapshots));
		    return;
	  case 4:
	  		persist_delete(PersistKeySnapshotArray4);
			persist_write_data(PersistKeySnapshotArray4, snapshots, sizeof(snapshots));
			return;
	  case 5:
			persist_delete(PersistKeySnapshotArray5);
			persist_write_data(PersistKeySnapshotArray5, snapshots, sizeof(snapshots));
			return;
	  default: 
	  		APP_LOG(APP_LOG_LEVEL_ERROR, "Array Number is out of bound: %d", arrayNumber);
	}

  //OLD METHOD
  //persist_write_int(PersistKeySnapshotIndexValue, snapshotIndexToBeWritten);
  //persist_write_data(PersistKeySnapshotArray1, snapshots, ((snapshotIndexToBeWritten + 1) * ((sizeof(Snapshot)))));
}

//Add message to message() and to persistent storage
static void queue_message(time_t timestamp, int steps, int experimentCondition, unsigned short causeOfSnapshot) {
  
  current_snapshotIndex++;	
  APP_LOG(APP_LOG_LEVEL_INFO, "current_snapshotIndex: %d", current_snapshotIndex);
 
  // Check Array Bounds - if over the size, we move the array into persistent storage, and clean current array
  if(current_snapshotIndex > MAX_SNAPSHOT_RAM){
	saveToPersistentStorage();
    current_snapshotIndex = 0;
	memset(&snapshots, 0x00, sizeof(snapshots));
  }
		
  //Creates new snapshot with current data and add to the snapshots array
  // Snapshot s = {timestamp, steps, experimentCondition, sideButtonPressed};
  Snapshot s = {timestamp, steps, causeOfSnapshot};
  snapshots[current_snapshotIndex] = s;
  
  // Null Terminate the snapshots array
  // JESSICA COMMENTED OUT
  //memset(&(snapshots[current_snapshotIndex + 1]), 0x00, sizeof(Snapshot));
  
  //Try to send the message
  //JESS: Create message if it fails
  message(snapshots,current_snapshotIndex); 
}

//Reason to send message happened --> Cause of Snapshot 
void comm_send_data(unsigned short causeOfSnapshot) {
  if (DEBUG) APP_LOG(APP_LOG_LEVEL_INFO, "Comm_Send_Steps called");
  time_t now = time(NULL);
  int steps = data_get_steps();
  mostRecentSnapshot = 1;
	
  //If it is outside of quiet hours --> Send message
  if(!data_server_moratorium_occurring_now()){
    queue_message(now, steps, experimentConditionThatUserIsIn, causeOfSnapshot);
  }
}

//When successful in sending message -- app_message_outbox_send() = APP_MSG_OK
// MAKE MODIFICATIONS
static void outbox_sent_handler(DictionaryIterator *iter, void *context) {
  	
  // JESSICA: CHECK IF LAST MESSAGE SENT WAS FROM PERMANENT STORAGE OR FROM SNAPSHOTS
  //CAN CREATE A FLAG THAT GOES TO 1 WHEN LEAVING MESSAGE AND A SEPARATE FLAG FOR THE PERSISTENT STORAGE
	APP_LOG(APP_LOG_LEVEL_INFO, "Sending message worked - outbox handler called");

	if(current_snapshot_sent) {
		current_snapshot_sent = false;
		current_snapshotIndex--;
		
		APP_LOG(APP_LOG_LEVEL_INFO, "Snapshot Index: %d (Sent handler)", current_snapshotIndex);
		// Null Terminate the snapshots array - remove last piece of data
  		memset(&(snapshots[current_snapshotIndex + 1]), 0x00, sizeof(Snapshot));
 
		//if there is a message, send it. Otherwise upload complete	
		if(current_snapshotIndex > -1) {
    		message(snapshots,current_snapshotIndex);
  		} else {
    		APP_LOG(APP_LOG_LEVEL_INFO, "Upload complete!");
  		}
	}
	
	if (persistent_snapshot_sent){
		persistent_snapshot_sent = false;
		persistent_snapshotIndex--;
		
		//JESSICA: Then send message of persistent snapshot Index and then when empty free the array and move on to the next one
		//when array is empty, call sendPersistentStorage() again so it continues on with the next item.
	}  
}

//If data received from the Server-- Seems to work
static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  
  if (DEBUG) APP_LOG(APP_LOG_LEVEL_INFO, "Inbox Received Handler Reached with: %d", current_snapshotIndex);
  
  Tuple *js_ready_t = dict_find(iter, AppKeyJSReady);
  if(js_ready_t) {
	//Why is this in the inbox received handler
    comm_send_data(0);
  }
  
  // Receive Fetched Experiment Condition
  Tuple *experimentCondition_returned_tuple = dict_find(iter, AppKeyReturnedExperimentConditionValue);
  if(experimentCondition_returned_tuple){
    if(experimentCondition_returned_tuple->value->int32){
      if (DEBUG) APP_LOG(APP_LOG_LEVEL_INFO, "Experiment Condition found in tuple: %d", (int)(experimentCondition_returned_tuple->value->int32));
      experimentConditionThatUserIsIn = experimentCondition_returned_tuple->value->int32;
      persist_write_int(PersistKeyFetchedExperimentConditionValue, (int)(experimentCondition_returned_tuple->value->int32));
      if(experimentConditionThatUserIsIn == 2){
        main_window_hide_steps();
      }
    }
  }
  
  // Receive Fetched Step Goal
  Tuple *stepGoal_returned_tuple = dict_find(iter, AppKeyReturnedStepGoalValue);
  if(stepGoal_returned_tuple){
    if(stepGoal_returned_tuple->value->int32){
      if (DEBUG) APP_LOG(APP_LOG_LEVEL_INFO, "Step Goal found in tuple: %d", (int)(stepGoal_returned_tuple->value->int32));
      usersStepGoal = stepGoal_returned_tuple->value->int32;
      persist_write_int(PersistKeyFetchedStepGoalValue, (int)(stepGoal_returned_tuple->value->int32));
    }
  }
}

// Loads the last array from persistent storage to the buffer
// Called at initialization 
bool loadPersistentStorage() {  
  //valueRead = persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
	
  /* Moved from initialization here;
  if (persist_exists(PersistKeySnapshotIndexValue)) {
    current_snapshotIndex = persist_read_int(PersistKeySnapshotIndexValue);
    if (DEBUG) APP_LOG(APP_LOG_LEVEL_INFO, "Current Snapshot index: %d", current_snapshotIndex);
  }*/
	
	/// WRITE A NUMBER OF ARRAYS WRITTEN KEY AND THEN DO A SWITCH CASE
  if (persist_exists(PersistKeyArrayNumber))
	  persist_write_int(PersistKeyArrayNumber, arrayNumber);
	else arrayNumber = 0;
	
  APP_LOG(APP_LOG_LEVEL_INFO, "Number of Arrays in Persistent memory: %d", arrayNumber);
	
  switch (arrayNumber) {
    case 0:
	      return false;
    case 1:
		  if (persist_exists(PersistKeySnapshotArray1))
			    persist_read_data(PersistKeySnapshotArray1, sbuffer, sizeof(sbuffer));
		  APP_LOG(APP_LOG_LEVEL_INFO, "Persistent Array 1");
          return 1;
	case 2:
		  if (persist_exists(PersistKeySnapshotArray2))
			    persist_read_data(PersistKeySnapshotArray2, sbuffer, sizeof(sbuffer));
		  APP_LOG(APP_LOG_LEVEL_INFO, "Persistent Array 2");
          return 1;
	case 3:
		  if (persist_exists(PersistKeySnapshotArray3))
			    persist_read_data(PersistKeySnapshotArray3, sbuffer, sizeof(sbuffer));
		  APP_LOG(APP_LOG_LEVEL_INFO, "Persistent Array 3");
    	  return 1;  
	case 4:
		  if (persist_exists(PersistKeySnapshotArray4))
			    persist_read_data(PersistKeySnapshotArray4, sbuffer, sizeof(sbuffer));
		  APP_LOG(APP_LOG_LEVEL_INFO, "Persistent Array 4");
          return 1;
	case 5:
		  if (persist_exists(PersistKeySnapshotArray5))
			    persist_read_data(PersistKeySnapshotArray5, sbuffer, sizeof(sbuffer));
		  APP_LOG(APP_LOG_LEVEL_INFO, "Persistent Array 5");
          return 1;
    default:
          APP_LOG(APP_LOG_LEVEL_INFO, "Did not find persistent storage");
	  	  return 0;
  }	  
}

// JESS: THIS FUNCTION READS FROM THE BUFFER AND SEND DATA AS MESSAGES
// IT IS CALLED AT INITIALIZATION AFTER LOAD AND AFTER BLUETOOTH RECONNECTION IF NOT EMPTY 
void sendPersistentStorage (){
	//if there was data in persisten storage, send it as messages
	//For each array
	//for(int i=0, i<arrayNumber, i++)
	//{
	  //ARRAY1 --> ARRAY 5
		// Takes the number of items inside the persistent stack
		persistent_snapshotIndex = (sizeof(sbuffer)/sizeof(Snapshot));
	    APP_LOG(APP_LOG_LEVEL_INFO, "Persistent snapshot Index %d", persistent_snapshotIndex);
		message(sbuffer,persistent_snapshotIndex);
	
		persistent_snapshot_sent = true;
		//return;
	//}
}

//Called at initialization
void comm_init(int inbox, int outbox) {
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_outbox_sent(outbox_sent_handler); //Initialization 
  app_message_open(inbox, outbox); //Here 64/128 -> size of buffer
  
  //JESSICA: INSTEAD I initialize the snapshots array at declaration with static --> That should be enough be to re-consider if i get memory allocation errors
  // Nullily Snapshots to start with
  //memset(snapshots, 0x00, sizeof(snapshots));
  	
  // Jessica NEW: At initialization load persistent storage  
  if(loadPersistentStorage()) sendPersistentStorage();
	else 
    APP_LOG(APP_LOG_LEVEL_INFO, "Nothing found in persistent storage");
}

//Created comm deinit function
void comm_deinit(){
	saveToPersistentStorage();
}
