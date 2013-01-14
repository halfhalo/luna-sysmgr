/* @@@LICENSE
*
*      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */


#include "Common.h"
#include <sys/prctl.h>

#include <string>
#include <map>
#include "BackupManager.h"
#include "Settings.h"
#include "HostBase.h"
#include "JSONUtils.h"
#include "Logging.h"
#include <cjson/json.h>

//for launcher3 saving
#include "pagesaver.h"

/* BackupManager implementation is based on the API documented at https://wiki.palm.com/display/ServicesEngineering/Backup+and+Restore+2.0+API
 * On the LunaSysMgr side, this backs up launcher, quick launch and dock mode settings
 */
BackupManager* BackupManager::s_instance = NULL;

/**
 * These are the methods that the backup service can call when it's doing a 
 * backup or restore.
 */
LSMethod BackupManager::s_BackupServerMethods[]  = {
	{ "preBackup"  , BackupManager::preBackupCallback },
	{ "postRestore", BackupManager::postRestoreCallback },
    { 0, 0 }
};


BackupManager::BackupManager() :
	m_mainLoop(NULL)
	, m_clientService(NULL)
	, m_serverService(NULL)
{
}

/**
 * Initialize the backup manager.
 */
bool BackupManager::init(GMainLoop* mainLoop)
{
    luna_assert(m_mainLoop == NULL);	// Only initialize once.
    m_mainLoop = mainLoop;

    LSError error;
    LSErrorInit(&error);
    char procName[100];

    // this service is expected to run from WebAppMgr process
    m_strBackupServiceName = "com.palm.sysMgrDataBackup";
    m_doBackupFiles = true;
    m_doBackupCookies = true;

    bool succeeded = LSRegisterPalmService(m_strBackupServiceName.c_str(), &m_serverService, &error);
    if (!succeeded) {
	g_warning("Failed registering on service bus: %s", error.message);
	LSErrorFree(&error);
	return false;
    }

    succeeded = LSPalmServiceRegisterCategory( m_serverService, "/", s_BackupServerMethods, NULL,
	    NULL, this, &error);
    if (!succeeded) {
	g_warning("Failed registering with service bus category: %s", error.message);
	LSErrorFree(&error);
	return false;
    }

    succeeded = LSGmainAttachPalmService(m_serverService, m_mainLoop, &error);
    if (!succeeded) {
	g_warning("Failed attaching to service bus: %s", error.message);
	LSErrorFree(&error);
	return false;
    }

    m_clientService = LSPalmServiceGetPrivateConnection(m_serverService);
    if (NULL == m_clientService) {
	g_warning("unable to get private handle to the backup service");
	return false;
    }

    return succeeded;
}

BackupManager::~BackupManager()
{
    if (m_serverService) {
	LSError error;
	LSErrorInit(&error);

	bool succeeded = LSUnregisterPalmService(m_serverService, &error);
	if (!succeeded) {
	    g_warning("Failed unregistering backup service: %s", error.message);
	    LSErrorFree(&error);
	}
    }
}


void BackupManager::initFilesForBackup()
{
//	if (!m_backupFiles.empty())
//		return;

	g_message("%s: entry",__FUNCTION__);
	m_backupFiles.clear();

    if (m_doBackupFiles) {
    	g_message("%s: adding files to backup list",__FUNCTION__);
		m_backupFiles.push_back ("/var/luna/preferences/launcher-cards.json");
		if (g_file_test(Settings::LunaSettings()->firstCardLaunch.c_str(), G_FILE_TEST_EXISTS))
			m_backupFiles.push_back (Settings::LunaSettings()->firstCardLaunch.c_str());
		if (g_file_test(Settings::LunaSettings()->quicklaunchUserPositions.c_str(), G_FILE_TEST_EXISTS))
			m_backupFiles.push_back (Settings::LunaSettings()->quicklaunchUserPositions.c_str());
		if (g_file_test(Settings::LunaSettings()->dockModeUserPositions.c_str(), G_FILE_TEST_EXISTS))
			m_backupFiles.push_back (Settings::LunaSettings()->dockModeUserPositions.c_str());

		QList<QString> fileList;

		DimensionsSystemInterface::PageSaver::filesForBackup(&fileList);
		for (QList<QString>::const_iterator file_it = fileList.constBegin();
				file_it != fileList.constEnd();file_it++)
		{
			if (!(file_it->isEmpty()))
			{
				if (g_file_test(file_it->toUtf8().constData(), G_FILE_TEST_EXISTS))
				{
					m_backupFiles.push_back(file_it->toUtf8().constData());
					g_message("%s: backing up signalled file: %s",__FUNCTION__,file_it->toUtf8().constData());
				}
			}
		}
    }
    else
    {
    	g_message("%s: DID NOT add files to backup list (backup files? flag was false)",__FUNCTION__);
    }
}

BackupManager* BackupManager::instance()
{
	if (NULL == s_instance) {
		s_instance = new BackupManager();
	}

	return s_instance;
}

bool BackupManager::preBackupCallback( LSHandle* lshandle, LSMessage *message, void *user_data)
{
	BackupManager* pThis = static_cast<BackupManager*>(user_data);
	luna_assert(pThis != NULL);

	// payload is expected to have the following fields - 
	// incrementalKey - this is used primarily for mojodb, backup service will handle other incremental backups
	// maxTempBytes - this is the allowed size of upload, currently 10MB (more than enough for our backups)
	// tempDir - directory to store temporarily generated files (currently unused by us)
	// - Since none of these are used now, we do not need to parse the payload


	// the response has to contain
	// description - what is being backed up
	// files - array of files to be backed up
	// version - version of the service
	struct json_object* response = json_object_new_object();
	if (!response) {
	    g_warning ("Unable to allocate json object");
	    return true;
	}

    json_object_object_add (response, "description", json_object_new_string ("Backup of LunaSysMgr files for launcher, quicklaunch and dockmode"));
	json_object_object_add (response, "version", json_object_new_string ("1.0"));

	// adding the files for backup at the time of request. 
	// if the user has created custom quicklaunch or dockmode settings, those files should be available now.
	pThis->initFilesForBackup();

	struct json_object* files = json_object_new_array();
	GFileTest fileTest = static_cast<GFileTest>(G_FILE_TEST_EXISTS|G_FILE_TEST_IS_REGULAR);

	if (pThis->m_doBackupFiles) {
	    std::list<std::string>::const_iterator i;
	    for (i = pThis->m_backupFiles.begin(); i != pThis->m_backupFiles.end(); ++i) {
			if (g_file_test(i->c_str(), fileTest)) {
				json_object_array_add (files, json_object_new_string(i->c_str()));
				g_debug ("added file %s to the backup list", i->c_str());
			}
	    }
	}

	json_object_object_add (response, "files", files);

	LSError lserror;
	LSErrorInit(&lserror);

	g_message ("Sending response to preBackupCallback: %s", json_object_to_json_string (response));
	if (!LSMessageReply (lshandle, message, json_object_to_json_string(response), &lserror )) {
	    g_warning("Can't send reply to preBackupCallback error: %s", lserror.message);
	    LSErrorFree (&lserror); 
	}

	json_object_put (response);
	return true;
}

bool BackupManager::postRestoreCallback( LSHandle* lshandle, LSMessage *message, void *user_data)
{
    BackupManager* pThis = static_cast<BackupManager*>(user_data);
    luna_assert(pThis != NULL);

    // {"files" : array}
    VALIDATE_SCHEMA_AND_RETURN(lshandle,
                               message,
                               SCHEMA_1(REQUIRED(files, array)));

    const char* str = LSMessageGetPayload(message);
    if (!str)
	return true;

    g_warning ("[BACKUPTRACE] %s: received %s", __func__, str);
    json_object* root = json_tokener_parse(str);
    if (!root || is_error(root))
	return true;

    json_object* files = json_object_object_get (root, "files");
    if (!files) {
	g_warning ("No files specified in postRestore message");
	return true;
    }

    // no work needed for regular files
    LSError lserror;
    LSErrorInit(&lserror);
    struct json_object* response = json_object_new_object();

    json_object_object_add (response, "returnValue", json_object_new_boolean(true));

    g_message ("Sending response to postRestoreCallback: %s", json_object_to_json_string (response));
    if (!LSMessageReply (lshandle, message, json_object_to_json_string(response), &lserror )) {
	g_warning("Can't send reply to postRestoreCallback error: %s", lserror.message);
	LSErrorFree (&lserror); 
    }

    json_object_put (response);
    return true;
}
