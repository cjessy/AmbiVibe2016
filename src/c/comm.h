#pragma once

#include "config.h"
#include "data.h"
#include "main_window.h"

extern int experimentConditionThatUserIsIn;
extern int usersStepGoal;
int finalStepCount; 
int mostRecentSnapshot;

typedef enum {
  AppKeySteps,
  AppKeyTimestamp,
  AppKeyJSReady,
  AppKeyReturnedExperimentConditionValue,
  AppKeyReturnedStepGoalValue,
  AppKeyExperimentCondition,
  AppKeyCauseOfSnapshot,
  AppKeyMostRecentSnapshot,
  AppKeyExperimentConditionSentForServer,
  AppKeyUsersStepGoal,
  AppKeyFinalSteps 
} AppKey;

//static void message(struct Snapshot[], int);

void saveToPersistentStorage();

static void queue_message(time_t timestamp, int steps, int experimentCondition, unsigned short causeOfSnapshot);

void comm_send_data(unsigned short causeOfSnapshot);

static void outbox_sent_handler(DictionaryIterator *iter, void *context);

static void inbox_received_handler(DictionaryIterator *iter, void *context);

bool loadPersistentStorage();

void sendPersistentStorage ();

void comm_init(int inbox, int outbox);

void comm_deinit();

