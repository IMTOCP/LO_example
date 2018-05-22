#pragma once
/*********************************************
* SUBSCRIBE TOPIC ON LO
*********************************************/

//SUBSCRIPTION CMD TOPIC
const char* cmdTopic = "dev/cmd";
//SUBSCRIPTION CONFIGURATION UPDATE TOPIC
const char* receiveCfgTopic = "dev/cfg/upd";

// SUBSCRIPTION OF DEVICE UPDATE RESOURCE TOPIC
const char* updtRscTopic = "dev/rsc/upd";

/*********************************************
* PUBLISH TOPIC ON LO
********************************************/

// PUBLICATION CMD TOPIC ACKNOWLEDGE OF RECEIPT
const char* cmdResTopic = "dev/cmd/res";
// PUBLICATION DEVICE INFO TOPIC 
const char* statusTopic = "dev/info";
// PUBLICATION DEVICE CONFIGURATION TOPIC
const char* postCurrentCfgTopic = "dev/cfg";
// PUBLICATION DEVICE METERING DATA TOPIC
const char* posDataTopic = "dev/data";
// PUBLICATION DEVICE RESOURCE TOPIC
const char* sendRscTopic = "dev/rsc";

// PUBLICATION DEVICE UPDATE RESOURCE TOPIC
const char* updtResponseRscTopic = "dev/rsc/upd/res";