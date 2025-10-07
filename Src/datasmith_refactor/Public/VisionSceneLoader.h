//
// Created by 25473 on 25-9-30.
//

#include <string>
#include <memory>
#include <sstream>
#include <ktm/ktm.h>

class VisionSceneLoader {

public:
    VisionSceneLoader();
    ~VisionSceneLoader();

    bool XmlParseFile(std::shared_ptr< Scene >& OutScene, bool bInAppend = false);
    bool PbrtParseFile(const std::string& InFilename, std::shared_ptr< Scene >& OutScene, bool bInAppend = false);
    bool jsonParseFile(const std::string& InFilename, std::shared_ptr< Scene >& OutScene, bool bInAppend = false);

private:

};

