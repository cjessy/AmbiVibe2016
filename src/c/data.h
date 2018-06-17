
#pragma once

#include "config.h"

typedef enum {
  PersistKeyFetchedExperimentConditionValue = 0,
  PersistKeyFetchedStepGoalValue = 1,
  PersistKeySnapshotArray1 = 2,
  PersistKeySnapshotArray2 = 3,
  PersistKeySnapshotArray3 = 4,
  PersistKeySnapshotArray4 = 5,
  PersistKeySnapshotArray5 = 6,
  PersistKeyArrayNumber = 7
  //PersistKeySnapshotIndexValue = 8,
} PersistKey;

int data_get_steps();

void updateLocalVariablesFromPersistentStorageValues();

int data_get_hourOfDay();

bool data_quiet_hours_occurring_now();

bool data_server_moratorium_occurring_now();