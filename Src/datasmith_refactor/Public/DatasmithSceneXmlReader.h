//
// Created by 25473 on 25-9-29.
//

#pragma once

#include <string>
#include <sstream>
#include <ktm/ktm.h>

#include "Transform.h"

class FDatasmithTextureSampler;
class FXmlFile;
class FXmlNode;
class UTexture;
struct FLinearColor;

class DatasmithSceneXmlReader
{
public:
	// Force non-inline constructor and destructor to prevent instantiating TUniquePtr< FXmlFile > with an incomplete FXmlFile type (forward declared)
	DatasmithSceneXmlReader() = default;
	~DatasmithSceneXmlReader() = default;

	bool ParseFile(const std::string& InFilename, std::shared_ptr< Scene >& OutScene, bool bInAppend = false);
	bool ParseBuffer(const std::string& XmlBuffer, std::shared_ptr< Scene >& OutScene, bool bInAppend = false);

private:
	bool ParseXmlFile(std::shared_ptr< Scene >& OutScene, bool bInAppend = false);
	void PatchUpVersion(std::shared_ptr< Scene >& OutScene) const;

	[[nodiscard]] std::string UnsanitizeXMLText(const std::string& InString) const;

	template<typename T> T ValueFromString(const std::string& InString) const = delete;
	ktm::fvec3 VectorFromNode(FXmlNode* InNode, const char* XName, const char* YName, const char* ZName) const;
	ktm::fquat QuatFromHexString(const std::string& HexString) const;
	ktm::fquat QuatFromNode(FXmlNode* InNode) const;
	Transform ParseTransform(FXmlNode* InNode) const;
	void ParseTransform(FXmlNode* InNode, std::shared_ptr< IDatasmithActorElement >& OutElement) const;

	void ParseElement(FXmlNode* InNode, std::shared_ptr<IDatasmithElement> OutElement) const;
	void ParseLevelSequence(FXmlNode* InNode, const std::shared_ptr<IDatasmithLevelSequenceElement>& OutElement) const;
	void ParseLevelVariantSets( FXmlNode* InNode, const std::shared_ptr<IDatasmithLevelVariantSetsElement>& OutElement, const TMap< std::string, std::shared_ptr<IDatasmithActorElement> >& Actors, const TMap< std::string, std::shared_ptr<IDatasmithElement> >& Objects ) const;
	void ParseVariantSet( FXmlNode* InNode, const std::shared_ptr<IDatasmithVariantSetElement>& OutElement, const TMap< std::string, std::shared_ptr<IDatasmithActorElement> >& Actors, const TMap< std::string, std::shared_ptr<IDatasmithElement> >& Objects ) const;
	void ParseVariant( FXmlNode* InNode, const std::shared_ptr<IDatasmithVariantElement>& OutElement, const TMap< std::string, std::shared_ptr<IDatasmithActorElement> >& Actors, const TMap< std::string, std::shared_ptr<IDatasmithElement> >& Objects ) const;
	void ParseActorBinding( FXmlNode* InNode, const std::shared_ptr<IDatasmithActorBindingElement>& OutElement, const TMap< std::string, std::shared_ptr<IDatasmithElement> >& Objects ) const;
	void ParsePropertyCapture( FXmlNode* InNode, const std::shared_ptr<IDatasmithPropertyCaptureElement>& OutElement ) const;
	void ParseObjectPropertyCapture( FXmlNode* InNode, const std::shared_ptr<IDatasmithObjectPropertyCaptureElement>& OutElement, const TMap< std::string, std::shared_ptr<IDatasmithElement> >& Objects ) const;
	void ParseMesh(FXmlNode* InNode, std::shared_ptr<IDatasmithMeshElement>& OutElement) const;
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	UE_DEPRECATED(5.5, "The experimental Cloth importer is no longer supported.")
	void ParseCloth(FXmlNode* InNode, std::shared_ptr<IDatasmithClothElement>& OutElement) const;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	void ParseTextureElement(FXmlNode* InNode, std::shared_ptr<IDatasmithTextureElement>& OutElement) const;
	void ParseTexture(FXmlNode* InNode, std::string& OutTextureFilename, FDatasmithTextureSampler& OutTextureUV) const;
	void ParseActor(FXmlNode* InNode, std::shared_ptr<IDatasmithActorElement>& InOutElement, std::shared_ptr< Scene > Scene, TMap< std::string, std::shared_ptr<IDatasmithActorElement> >& Actors) const;
	void ParseMeshActor(FXmlNode* InNode, std::shared_ptr<IDatasmithMeshActorElement>& OutElement, std::shared_ptr< Scene > Scene) const;
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	UE_DEPRECATED(5.5, "The experimental Cloth importer is no longer supported.")
	void ParseClothActor(FXmlNode* InNode, std::shared_ptr<IDatasmithClothActorElement>& OutElement, std::shared_ptr< Scene > Scene) const;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	void ParseHierarchicalInstancedStaticMeshActor(FXmlNode* InNode, std::shared_ptr<IDatasmithHierarchicalInstancedStaticMeshActorElement>& OutElement, std::shared_ptr< Scene > Scene) const;
	void ParseLight(FXmlNode* InNode, std::shared_ptr<IDatasmithLightActorElement>& OutElement) const;
	void ParseCamera(FXmlNode* InNode, std::shared_ptr<IDatasmithCameraActorElement>& OutElement) const;
	void ParsePostProcess(FXmlNode* InNode, const std::shared_ptr< IDatasmithPostProcessElement >& Element) const;
	void ParsePostProcessVolume(FXmlNode* InNode, const std::shared_ptr< IDatasmithPostProcessVolumeElement >& Element) const;
	void ParseColor(FXmlNode* InNode, FLinearColor& OutColor) const;
	void ParseComp(FXmlNode* InNode, std::shared_ptr< IDatasmithCompositeTexture >& OutCompTexture, bool bInIsNormal = false) const;
	void ParseMaterial(FXmlNode* InNode, std::shared_ptr< IDatasmithMaterialElement >& OutElement) const;
	void ParseMaterialInstance(FXmlNode* InNode, std::shared_ptr< IDatasmithMaterialInstanceElement >& OutElement) const;
	void ParseDecalMaterial(FXmlNode* InNode, std::shared_ptr< IDatasmithDecalMaterialElement >& OutElement) const;
	void ParseUEPbrMaterial(FXmlNode* InNode, std::shared_ptr< IDatasmithUEPbrMaterialElement >& OutElement) const;
	void ParseCustomActor(FXmlNode* InNode, std::shared_ptr< IDatasmithCustomActorElement >& OutElement) const;
	void ParseMetaData(FXmlNode* InNode, std::shared_ptr< IDatasmithMetaDataElement >& OutElement, const std::shared_ptr< Scene >& InScene, TMap< std::string, std::shared_ptr<IDatasmithActorElement> >& Actors) const;
	void ParseLandscape(FXmlNode* InNode, std::shared_ptr< IDatasmithLandscapeElement >& OutElement) const;

	template< typename ElementType >
	void ParseKeyValueProperties(const FXmlNode* InNode, ElementType& OutElement) const;

	bool LoadFromFile(const std::string& InFilename);
	bool LoadFromBuffer(const std::string& XmlBuffer);

	template< typename ExpressionInputType >
	void ParseExpressionInput(const FXmlNode* InNode, std::shared_ptr< IDatasmithUEPbrMaterialElement >& OutElement, ExpressionInputType& ExpressionInput) const;

	TUniquePtr< FXmlFile > XmlFile;
	std::string ProjectPath;
};

