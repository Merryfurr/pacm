//
// LibSourcey
// Copyright (C) 2005, Sourcey <http://sourcey.com>
//
// LibSourcey is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// LibSourcey is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//


#ifndef SCY_Pacm_PackageManager_H
#define SCY_Pacm_PackageManager_H


#include "scy/pacm/config.h"
#include "scy/pacm/types.h"
#include "scy/pacm/package.h"
#include "scy/pacm/installtask.h"
#include "scy/pacm/installmonitor.h"
#include "scy/collection.h"
#include "scy/filesystem.h"
#include "scy/platform.h"
#include "scy/stateful.h"
#include "scy/taskrunner.h"
#include "scy/json/json.h"


#include <iostream>
#include <fstream>
#include <assert.h>


namespace scy { 
namespace pacm {


typedef LiveCollection<std::string, LocalPackage>    LocalPackageStore;
typedef LocalPackageStore::Map                        LocalPackageMap;
typedef LiveCollection<std::string, RemotePackage>    RemotePackageStore;
typedef RemotePackageStore::Map                        RemotePackageMap;


class PackageManager
    /// The Package Manager provides an interface for managing, 
    /// installing, updating and uninstalling Pacm packages.
{
public:
    struct Options 
        // Package manager initialization options.
    {
        std::string endpoint;           // The HTTP server endpoint
        std::string indexURI;           // The HTTP server URI for querying packages JSON
        std::string httpUsername;       // Username for HTTP basic auth
        std::string httpPassword;       // PAssword for HTTP basic auth
        std::string httpOAuthToken;     // Will be used instead of HTTP basic if provided
        
        std::string tempDir;            // Directory where package files will be downloaded and extracted
        std::string dataDir;            // Directory where package manifests will be kept
        std::string installDir;         // Directory where packages will be installed
                
        std::string platform;           // Platform (win32, linux, mac)
        std::string checksumAlgorithm;  // Checksum algorithm (MDS/SHA1)

        bool clearFailedCache;          // This flag tells the package manager weather or not
                                        // to clear the package cache if installation fails.

        Options(const std::string& root = getCwd()) {    
            tempDir                    = root + fs::separator + DEFAULT_PACKAGE_TEMP_DIR;
            dataDir                    = root + fs::separator + DEFAULT_PACKAGE_DATA_DIR;
            installDir                = root + fs::separator + DEFAULT_PACKAGE_INSTALL_DIR;
            endpoint                = DEFAULT_API_ENDPOINT;
            indexURI                = DEFAULT_API_INDEX_URI;
            platform                = DEFAULT_PLATFORM;
            checksumAlgorithm       = DEFAULT_CHECKSUM_ALGORITHM;
            clearFailedCache        = true;
        }
    };

public:    
    PackageManager(const Options& options = Options());
    virtual ~PackageManager();
    
    //
    /// Initialization Methods

    virtual void initialize();
    virtual void uninitialize();

    virtual bool initialized() const;

    virtual void createDirectories();
        // Creates the package manager directory structure
        // if it does not already exist.
    
    virtual void queryRemotePackages();
        // Queries the server for a list of available packages.

    virtual void loadLocalPackages();
        // Loads all local package manifests from file system.
        // Clears all in memory package manifests. 

    virtual void loadLocalPackages(const std::string& dir);
        // Loads all local package manifests residing the the
        // given directory. This method may be called multiple
        // times for different paths because it does not clear
        // in memory package manifests. 
    
    virtual bool saveLocalPackages(bool whiny = false);
    virtual bool saveLocalPackage(LocalPackage& package, bool whiny = false);
        // Saves the local package manifest to the file system.
    
    //
    /// Package Installation Methods

    virtual InstallTask::Ptr installPackage(const std::string& name,
        const InstallOptions& options = InstallOptions()); //, bool whiny = false
        // Installs a single package.
        // The returned InstallTask must be started.
        // If the package is already up-to-date, a nullptr will be returned.
        // Any other error will throw a std::runtime_error.

    virtual bool installPackages(const StringVec& ids, 
        const InstallOptions& options = InstallOptions(),
        InstallMonitor* monitor = nullptr, bool whiny = false);
        // Installs multiple packages.
        // The same options will be passed to each task.
        // If a InstallMonitor instance was passed in the tasks will need to
        // be started, otherwise they will be auto-started.
        // The PackageManager does not take ownership of the InstallMonitor.    

    virtual InstallTask::Ptr updatePackage(const std::string& name, 
        const InstallOptions& options = InstallOptions()); //, bool whiny = false
        // Updates a single package.
        // Throws an exception if the package does not exist.
        // The returned InstallTask must be started.

    virtual bool updatePackages(const StringVec& ids, 
        const InstallOptions& options = InstallOptions(), 
        InstallMonitor* monitor = nullptr, bool whiny = false);
        // Updates multiple packages.
        // Throws an exception if the package does not exist.
        // If a InstallMonitor instance was passed in the tasks will need to
        // be started, otherwise they will be auto-started.
        // The PackageManager does not take ownership of the InstallMonitor.    

    virtual bool updateAllPackages(bool whiny = false);
        // Updates all installed packages.

    virtual bool uninstallPackages(const StringVec& ids, bool whiny = false);
        // Uninstalls multiple packages.

    virtual bool uninstallPackage(const std::string& id, bool whiny = false);
        // Uninstalls a single package.

    virtual bool hasUnfinalizedPackages();
        // Returns true if there are updates available that have
        // not yet been finalized. Packages may be unfinalized if
        // there were files in use at the time of installation.
            
    virtual bool finalizeInstallations(bool whiny = false);
        // Finalizes active installations by moving all package
        // files to their target destination. If files are to be
        // overwritten they must not be in use or finalization
        // will fail.    
        
    //
    /// Task Helper Methods

    virtual InstallTask::Ptr getInstallTask(const std::string& id) const;
        // Gets the install task for the given package ID.

    virtual InstallTaskPtrVec tasks() const;
        // Returns a list of all tasks.
    
    virtual void cancelAllTasks();
        // Aborts all package installation tasks. All tasks must
        // be aborted before clearing local or remote manifests.

    //
    /// Package Helper Methods

    virtual PackagePairVec getPackagePairs() const;
        // Returns all package pairs, valid or invalid.
        // Some pairs may not have both local and remote package pointers.
    
    virtual PackagePairVec getUpdatablePackagePairs() const;
        // Returns a list of package pairs which may be updated.
        // All pairs will have both local and remote package pointers,
        // and the remote version will be newer than the local version.    

    virtual PackagePair getPackagePair(const std::string& id, bool whiny = false) const;
        // Returns a local and remote package pair.
        // An exception will be thrown if either the local or
        // remote packages aren't available or are invalid.    
    
    virtual PackagePair getOrCreatePackagePair(const std::string& id);
        // Returns a local and remote package pair.
        // If the local package doesn't exist it will be created
        // from the remote package.
        // If the remote package doesn't exist a NotFoundException 
        // will be thrown.    
    
    virtual InstallTask::Ptr createInstallTask(PackagePair& pair, 
        const InstallOptions& options = InstallOptions());
        // Creates a package installation task for the given pair.
    
    virtual std::string installedPackageVersion(const std::string& id) const;
        // Returns the version number of an installed package.
        // Exceptions will be thrown if the package does not exist,
        // or is not fully installed.
        
    virtual Package::Asset getLatestInstallableAsset(const PackagePair& pair, 
        const InstallOptions& options = InstallOptions()) const;
        // Returns the best asset to install, or throws a descriptive exception
        // if no updates are available, or if the package is already up-to-date.
        // This method takes version and SDK locks into consideration.
        
    virtual bool hasAvailableUpdates(const PackagePair& pair) const;
        // Returns true if there are updates available for this package, false otherwise.
        
    //
    /// File Helper Methods

    void clearCache();
        // Clears all files in the cache directory.

    bool clearPackageCache(LocalPackage& package);
        // Clears a package archive from the local cache.

    bool clearCacheFile(const std::string& fileName, bool whiny = false);
        // Clears a file from the local cache.

    bool hasCachedFile(Package::Asset& asset);
        // Checks if a package archive exists in the local cache.
    
    bool isSupportedFileType(const std::string& fileName);
        // Checks if the file type is a supported package archive.
    
    std::string getCacheFilePath(const std::string& fileName);
        // Returns the full path of the cached file if it exists,
        // or an empty path if the file doesn't exist.

    std::string getPackageDataDir(const std::string& id);
        // Returns the package data directory for the
        // given package ID.
    
    // 
    /// Accessors

    virtual Options& options();
    virtual RemotePackageStore& remotePackages();
    virtual LocalPackageStore& localPackages();
        
    //
    /// Events

    Signal<const http::Response&> RemotePackageResponse;
        // Fires when the remote package list have been 
        // downloaded from the server.

    Signal<LocalPackage&> PackageUninstalled;
        // Fires when a package is uninstalled.
    
    Signal<InstallTask&> InstallTaskCreated;
        // Fires when an installation task is created, 
        // before it is started.

    Signal<const InstallTask&> InstallTaskComplete;
        // fires when a package installation tasks completes, 
        // either successfully or in error.

protected:

    //
    /// Callbacks

    void onPackageInstallComplete(void* sender);

    void onPackageQueryResponse(void* sender, const http::Response& response);
    
protected:    
    mutable Mutex       _mutex;
    LocalPackageStore    _localPackages;
    RemotePackageStore    _remotePackages;
    InstallTaskPtrVec    _tasks;
    Options                _options;
};



} } // namespace scy::pacm


#endif // SCY_Pacm_PackageManager_H