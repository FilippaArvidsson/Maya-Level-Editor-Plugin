//Uppgift 2 UD1447 2020-09-16

//Filippa Arvidsson

#include "Includes.h"
#include <maya/MTimer.h>
#include <algorithm>
#include <vector>
#include <queue>

#include "ComLib.h"

using namespace std;

struct MaterialCallbackData
{
	//One per mesh
	char m_meshName[50];
	MCallbackId m_callbackID;
	char m_materialName[50];


	int hasTextureFileCallback = -1;
	MCallbackId m_textureFileCallbackID;
};

MTimer mTimer;
double mElapsedTime = 0.0f;
MStatus status = MStatus::kSuccess;
MObject m_node;
MCallbackIdArray callbackIdArray;
ComLib* producer = nullptr;

std::vector<MaterialCallbackData> matCallbackData;

//Spam-Tendency Messages
std::vector<std::vector<ViewerMessage::TransformMessage>> transformMessages;
std::vector<ViewerMessage::CameraMessage> cameraMessages;
std::vector<ViewerMessage::CameraTransformMessage> cameraTransformMessages;
std::vector<std::vector<ViewerMessage::SingleVertexMessage>> singleVertexMessages;

//PROTOTYPES
void nodeRenamed(MObject& node, const MString& str, void* clientData);

void tempAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void attributeChangedTransform(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void attributeChangedMesh(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void topologyModified(MObject& node, void* clientData);

void attributeChangedMaterial(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void attributeChangedFile(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

void timerCallback(float elapsedTime, float lastTime, void* clientData);

//MESSAGE DATA
int transformMessageForMeshExists(char meshName[])
{
	int result = -1;

	for (int i = 0; i < transformMessages.size(); i++)
	{
		if (strcmp(transformMessages[i][0].m_meshName, meshName) == 0)
		{
			result = i;
			break;
		}
	}

	return result;
}

std::vector<int> singleVertexMessageIndexExists(char meshName[], int index)
{
	std::vector<int> result;

	result.resize(2);

	result[0] = -1;
	result[1] = -1;

	for (int i = 0; i < singleVertexMessages.size(); i++)
	{
		if (strcmp(singleVertexMessages[i][0].m_meshName, meshName) == 0)
		{
			result[0] = i;
			for (int j = 0; j < singleVertexMessages[i].size(); j++)
			{
				if (index == singleVertexMessages[i][j].m_index)
				{
					result[1] = j;
					break;
				}
			}
		}
	}

	return result;
}

int getMaterialCallbackDataIndexByMesh(char meshName[])
{
	int result = -1;

	for (int i = 0; i < matCallbackData.size(); i++)
	{
		if (strcmp(matCallbackData[i].m_meshName, meshName) == 0)
		{
			result = i;
			break;
		}
	}

	return result;
}

std::vector<int> getMaterialCallbackDataIndicesByMaterial(char materialName[])
{
	std::vector<int> result;

	for (int i = 0; i < matCallbackData.size(); i++)
	{
		if (strcmp(matCallbackData[i].m_materialName, materialName) == 0)
		{
			result.push_back(i);
		}
	}

	return result;
}

MIntArray globalToLocalVertexIndex(MIntArray& vertices, MIntArray& triangles)
{
	MIntArray localVertexIndex;

	//For every triangle
	for (int i = 0; i < triangles.length(); i++)
	{
		//For every vertex in triangle
		for (int j = 0; j < vertices.length(); j++)
		{
			if (triangles[i] == vertices[j])
			{
				//If object-relative index = face-relative index

				//Add index to final array
				localVertexIndex.append(j);
				break;
			}
		}
	}

	return localVertexIndex;
}

void getMeshVertexdata(MObject node, std::vector<ViewerMessage::VertexMessage>& vertexMessages)
{
	MStatus res;

	MFnMesh mesh(node, &res);

	if (res == MStatus::kSuccess)
	{
		//Position
		MPointArray vtxPositions;
		mesh.getPoints(vtxPositions);
		
		//UV Sets
		MStringArray UVsets;
		mesh.getUVSetNames(UVsets);

		//UV
		MFloatArray u, v;
		mesh.getUVs(u, v, &UVsets[0]);

		//Normal
		MFloatVectorArray vtxNormals;
		mesh.getNormals(vtxNormals);

		MItMeshPolygon itMP(node, &res);

		if (res == MStatus::kSuccess)
		{
			int nrOfVertices = 0;

			while (!itMP.isDone())
			{
				//Indices
				MIntArray indices;
				itMP.getVertices(indices);

				MIntArray localIndex;

				//Get Triangulation
				int nrOfTriangles = 0;
				itMP.numTriangles(nrOfTriangles);

				ViewerMessage::VertexMessage vertex;

				while (nrOfTriangles--)
				{
					MPointArray points;
					MIntArray triangleIndices;

					res = itMP.getTriangle(nrOfTriangles, points, triangleIndices);

					if (res == MStatus::kSuccess)
					{
						int uvID[3];

						for (int i = 0; i < 3; i++)
						{
							//Position

							std::cout << "Index: " << nrOfVertices << std::endl;

							vertex.m_position[0] = (float)points[i].x;
							vertex.m_position[1] = (float)points[i].y;
							vertex.m_position[2] = (float)points[i].z;

							localIndex = globalToLocalVertexIndex(indices, triangleIndices);

							//Normal
							vertex.m_normal[0] = (float)vtxNormals[itMP.normalIndex(i)].x;
							vertex.m_normal[1] = (float)vtxNormals[itMP.normalIndex(i)].y;
							vertex.m_normal[2] = (float)vtxNormals[itMP.normalIndex(i)].z;

							std::cout << "Normal: X: " << vertex.m_normal[0] << " Y: " << vertex.m_normal[1] << " Z: " << vertex.m_normal[2] << std::endl;

							//UV
							itMP.getUVIndex(localIndex[i], uvID[i], &UVsets[0]);

							vertex.m_uv[0] = (float)u[uvID[i]];
							vertex.m_uv[1] = (float)v[uvID[i]];

							std::cout << "UV: U: " << vertex.m_uv[0] << " : " << vertex.m_uv[1] << std::endl;

							//Push Back
							vertex.m_index = nrOfVertices;

							vertexMessages.push_back(vertex);
							nrOfVertices++;
						}
					}
				}

				itMP.next();
			}
		}


	}

}

void getChildMatrix(MObject child)
{
	MStatus res;

	MFnDagNode dag(child, &res);

	if (res == MStatus::kSuccess)
	{
		int numChildren = dag.childCount(&res);

		for (int i = 0; i < numChildren; i++)
		{
			getChildMatrix(dag.child(i, &res));
		}

		//Function
		if (child.hasFn(MFn::kTransform))
		{

			//CHILD
			MFnTransform childT(child);

			//GLOBAL
			MMatrix cTM = childT.transformationMatrix(&res);

			MFnMatrixData matrixDataChild(childT.findPlug("parentMatrix", true, &res).elementByLogicalIndex(0).asMObject());
			MMatrix pTM = matrixDataChild.matrix(&res);

			MMatrix finalTM = cTM * pTM;

			float destC[4][4];

			finalTM.get(destC);

			//Prepare Transform Message
			ViewerMessage::BaseMessage baseMSG;
			ViewerMessage::TransformMessage transformMSG;

			int count = 0;

			for (int i = 0; i < 4; i++)
			{

				for (int j = 0; j < 4; j++)
				{
					transformMSG.m_matrix[count] = destC[i][j];
					count++;
				}

			}

			//Get Mesh Name
			MDagPath dagPath;

			res = dagPath.getAPathTo(child, dagPath);
			res = dagPath.extendToShape();

			MFnMesh tMesh(dagPath.node(), &res);

			if (res == MStatus::kSuccess)
			{
				baseMSG.m_msgType = ViewerMessage::MSG_TYPE::Transform;
				baseMSG.m_messageSize = sizeof(ViewerMessage::TransformMessage);
				strcpy_s(transformMSG.m_meshName, tMesh.name().asChar());

				int index = transformMessageForMeshExists(transformMSG.m_meshName);

				if (index != -1)
				{
					transformMessages[index].push_back(transformMSG);
				}
				else
				{
					std::vector<ViewerMessage::TransformMessage> empty;

					empty.push_back(transformMSG);

					transformMessages.push_back(empty);
				}

			}


		}
	}
}

//CHILD ADDED
void childAdded(MDagPath& child, MDagPath& parent, void* clientData)
{
	MStatus res;

	MFnDependencyNode dep(child.node(), &res);
	MFnDependencyNode depN(child.node());

	if (child.node().hasFn(MFn::kMesh))
	{
		MObject obj = child.node();

		MNodeMessage::addAttributeChangedCallback(obj, tempAttributeChanged, NULL, &status);
		callbackIdArray.append(MNodeMessage::addNameChangedCallback(obj, nodeRenamed, NULL, &status));
	}
}

void nodeRenamed(MObject& node, const MString& str, void* clientData)
{
	MStatus res;
	MFnDependencyNode nodeFn(node, &res);

	if (res == MStatus::kSuccess)
	{
		//MESH
		if (node.hasFn(MFn::kMesh))
		{
			std::string nodeName = nodeFn.name().asChar();

			ViewerMessage::BaseMessage baseMSG;
			ViewerMessage::RenamedMessage renamedMSG;

			//Base Message
			baseMSG.m_messageSize = sizeof(ViewerMessage::RenamedMessage);
			baseMSG.m_msgType = ViewerMessage::MSG_TYPE::Rename;

			//Renamed Message
			strcpy_s(renamedMSG.m_oldName, str.asChar());
			strcpy_s(renamedMSG.m_newName, nodeName.c_str());

			while (!producer->producerCanWrite(baseMSG))
			{

			}

			producer->writeRenamedMessage(baseMSG, renamedMSG);

			std::cout << "Wrote Renamed Message" << std::endl;
			std::cout << "Offset: " << producer->getOffset() << std::endl;
			std::cout << "Other Offset: " << producer->getOtherOffset() << std::endl;

			//Update Material Callback Data
			int index = getMaterialCallbackDataIndexByMesh(renamedMSG.m_oldName);

			if (index != -1)
			{
				strcpy_s(matCallbackData[index].m_meshName, renamedMSG.m_newName);
			}
		}
		//MATERIAL
		else if (node.hasFn(MFn::kLambert))
		{
			char materialName[50];
			strcpy_s(materialName, str.asChar());
			std::vector<int> indices = getMaterialCallbackDataIndicesByMaterial(materialName);

			for (int i = 0; i < indices.size(); i++)
			{
				strcpy_s(matCallbackData[indices[i]].m_materialName, nodeFn.name().asChar());
			}
		}
		

	}

}

void nodeDeleted(MObject& node, void* clientData)
{
	MStatus res;

	if (node.hasFn(MFn::kMesh))
	{

		MFnDependencyNode depNode(node, &res);

		if (res == MStatus::kSuccess)
		{
			ViewerMessage::BaseMessage baseMSG;
			ViewerMessage::DeletedMessage deletedMessage;

			baseMSG.m_msgType = ViewerMessage::MSG_TYPE::Delete;
			baseMSG.m_messageSize = sizeof(ViewerMessage::DeletedMessage);

			strcpy_s(deletedMessage.m_deletedMeshName, depNode.name().asChar());

			//Send Message
			while (!producer->producerCanWrite(baseMSG))
			{

			}

			producer->writeDeletedMessage(baseMSG, deletedMessage);


			std::cout << "Wrote Deleted Message" << std::endl;
			std::cout << "Offset: " << producer->getOffset() << std::endl;
			std::cout << "Other Offset: " << producer->getOtherOffset() << std::endl;
		}

	}
}

void tempAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	MStatus res;

	if (plug.node().hasFn(MFn::kMesh))
	{

		MFnDependencyNode dep(plug.node(), &res);

		MFnMesh mesh(plug.node(), &res);

		if (res == MStatus::kSuccess)
		{

			MPointArray vertices;
			MIntArray vertexList;
			MIntArray vertexCount;

			std::vector<ViewerMessage::VertexMessage> vertexMessages;

			getMeshVertexdata(plug.node(), vertexMessages);

			if (res == MStatus::kSuccess)
			{
				int numChildren = vertices.length();

				ViewerMessage::BaseMessage baseMSG(ViewerMessage::MSG_TYPE::Base);
				ViewerMessage::MeshMessage meshMSG;

				meshMSG.m_nrOfVertices = vertexMessages.size();
				strcpy_s(meshMSG.m_meshName, mesh.name().asChar());

				baseMSG.m_msgType = ViewerMessage::MSG_TYPE::Mesh;
				baseMSG.m_messageSize = sizeof(ViewerMessage::MeshMessage) + (sizeof(ViewerMessage::VertexMessage) * vertexMessages.size());

				while (!producer->producerCanWrite(baseMSG))
				{

				}

				producer->writeMeshMessage(baseMSG, meshMSG, vertexMessages);

				std::cout << "Wrote Mesh Message" << std::endl;
				std::cout << "Offset: " << producer->getOffset() << std::endl;
				std::cout << "Other Offset: " << producer->getOtherOffset() << std::endl;

				MMessage::removeCallback(MMessage::currentCallbackId());

				MObject obj = plug.node();
				callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(obj, attributeChangedMesh, NULL, &status));
				callbackIdArray.append(MNodeMessage::addNodePreRemovalCallback(obj, nodeDeleted, NULL, &status));
				callbackIdArray.append(MPolyMessage::addPolyTopologyChangedCallback(obj, topologyModified, NULL, &status));


				MString myCommand = "setAttr - e " + mesh.name() + ".quadSplit 0;";
				MGlobal::executeCommandOnIdle(myCommand);

			}
		}
	}

	
}

void attributeChangedMesh(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	MStatus res;

	if (msg & MNodeMessage::kAttributeSet)
	{

		MFnDependencyNode depNode(plug.node(), &res);

		if (plug.isElement() && plug == depNode.findPlug("pnts", true, &res))
		{

			//Vertex Moved

			std::string info = plug.info(&res).asChar();

			if (res == MStatus::kSuccess)
			{

				int index = plug.logicalIndex(&res);

				MFnMesh mNode(plug.node(), &res);

				if (res == MStatus::kSuccess)
				{
					//Normal
					MFloatVectorArray vtxNormals;
					mNode.getNormals(vtxNormals);

					MStringArray UVsets;
					mNode.getUVSetNames(UVsets);

					MFloatArray u, v;
					mNode.getUVs(u, v, &UVsets[0]);

					MItMeshPolygon itMP(plug.node(), &res);

					if (res == MStatus::kSuccess)
					{
						int nrOfVertices = 0;

						ViewerMessage::BaseMessage baseMSG;
						baseMSG.m_messageSize = sizeof(ViewerMessage::SingleVertexMessage);
						baseMSG.m_msgType = ViewerMessage::MSG_TYPE::SingleVertex;

						while (!itMP.isDone())
						{

							//Get Triangulation
							int nrOfTriangles = 0;
							itMP.numTriangles(nrOfTriangles);

							MIntArray localIndex;

							MIntArray indices;
							itMP.getVertices(indices);

							//For every triangle in this polygon...
							while (nrOfTriangles--)
							{

								MPointArray points;
								MIntArray triangleIndices;

								res = itMP.getTriangle(nrOfTriangles, points, triangleIndices);

								if (res == MStatus::kSuccess)
								{
									int uvID[3];

									//For every vertex in triangle...
									for (int i = 0; i < 3; i++)
									{

										if (index == triangleIndices[i])
										{
											ViewerMessage::SingleVertexMessage singleVertexMSG;

											singleVertexMSG.m_index = nrOfVertices;

											std::cout << "Index: " << nrOfVertices << std::endl;

											singleVertexMSG.m_position[0] = (float)points[i].x;
											singleVertexMSG.m_position[1] = (float)points[i].y;
											singleVertexMSG.m_position[2] = (float)points[i].z;

											localIndex = globalToLocalVertexIndex(indices, triangleIndices);

											//Normal
											singleVertexMSG.m_normal[0] = (float)vtxNormals[itMP.normalIndex(i)].x;
											singleVertexMSG.m_normal[1] = (float)vtxNormals[itMP.normalIndex(i)].y;
											singleVertexMSG.m_normal[2] = (float)vtxNormals[itMP.normalIndex(i)].z;

											//UV
											itMP.getUVIndex(localIndex[i], uvID[i], &UVsets[0]);

											singleVertexMSG.m_uv[0] = (float)u[uvID[i]];
											singleVertexMSG.m_uv[1] = (float)v[uvID[i]];

											std::cout << "UV: U: " << singleVertexMSG.m_uv[0] << " : " << singleVertexMSG.m_uv[1] << std::endl;

											strcpy_s(singleVertexMSG.m_meshName, mNode.name().asChar());

											//Save Message

											std::vector<int> result = singleVertexMessageIndexExists(singleVertexMSG.m_meshName, singleVertexMSG.m_index);

											if (result[0] != -1)
											{
												//Mesh already exists

												if (result[1] != -1)
												{
													//Vertex Already Exists

													//Overwrite with latest
													singleVertexMessages[result[0]][result[1]] = singleVertexMSG;

												}
												else
												{
													//Mesh exists but vertex doesn't

													singleVertexMessages[result[0]].push_back(singleVertexMSG);
												}
												

											}
											else
											{
												//Mesh doesn't exist yet
												std::vector<ViewerMessage::SingleVertexMessage> empty;

												empty.push_back(singleVertexMSG);

												singleVertexMessages.push_back(empty);

											}

										}

										nrOfVertices++;
									}

								}
							}

							itMP.next();

						}
					}

				}
			}
		}
	}
	else if (msg & MNodeMessage::kOtherPlugSet)
	{
		std::cout << "Mesh Attribute Other Plug Set!!" << std::endl;


		MFnDependencyNode depNode(plug.node(), &res);

		if (plug == depNode.findPlug("instObjGroups", true, &res))
		{
			MFnDependencyNode otherDepNode(otherPlug.node(), &res);

			MPlug shaderPlug = otherDepNode.findPlug("surfaceShader", true, &res);

			std::cout << "Shader Plug: " << shaderPlug.name().asChar() << std::endl;

			MPlugArray plugs;
			shaderPlug.connectedTo(plugs, true, false, &res);

			//Material Set
			MFnLambertShader lShader(plugs[0].node(), &res);

			if (res == MStatus::kSuccess)
			{
				std::cout << "Message Sending " << std::endl;

				ViewerMessage::BaseMessage baseMSG;
				ViewerMessage::MaterialMessage materialMSG;

				baseMSG.m_msgType = ViewerMessage::MSG_TYPE::Material;
				baseMSG.m_messageSize = sizeof(ViewerMessage::MaterialMessage);

				strcpy_s(materialMSG.m_meshName, depNode.name().asChar());

				//Determine if textured or colored

				//Diffuse Texture

				MPlug colorPlug = lShader.findPlug("color", true, &res);
				std::cout << "Color Plug: " << colorPlug.name().asChar() << std::endl;

				MPlugArray connectedPlugs;
				colorPlug.connectedTo(connectedPlugs, true, false, &res);

				if (res == MStatus::kSuccess && connectedPlugs.length() > 0)
				{
					std::cout << "Plug Connected To Color: " << connectedPlugs[0].name().asChar() << std::endl;
					std::cout << std::endl;
					if (connectedPlugs[0].node().hasFn(MFn::kFileTexture))
					{
						std::cout << "Has a diffuse texture" << std::endl;
						std::cout << std::endl;
						materialMSG.m_shadingType = ViewerMessage::SHADING_TYPE::TEXTURED;

						MFnDependencyNode fileNode(connectedPlugs[0].node(), &res);

						MPlug filePathPlug = fileNode.findPlug("fileTextureName", true, &res);

						if (res == MStatus::kSuccess)
						{
							MString filePath;
							filePathPlug.getValue(filePath);
							std::cout << filePath.asChar() << std::endl;
							strcpy_s(materialMSG.m_diffTexturePath, filePath.asChar());
						}

					}
					else
					{
						materialMSG.m_shadingType = ViewerMessage::SHADING_TYPE::COLORED;
					}
				}

				MColor currentColor;
				currentColor = lShader.ambientColor(&res);

				materialMSG.m_ambient[0] = currentColor.r;
				materialMSG.m_ambient[1] = currentColor.g;
				materialMSG.m_ambient[2] = currentColor.b;
				materialMSG.m_ambient[3] = currentColor.a;

				currentColor = lShader.color(&res);

				materialMSG.m_diffuse[0] = currentColor.r;
				materialMSG.m_diffuse[1] = currentColor.g;
				materialMSG.m_diffuse[2] = currentColor.b;

				currentColor = lShader.transparency(&res);

				materialMSG.m_diffuse[3] = 1 - currentColor.r;
				
				//Send Messages
				
				while (!producer->producerCanWrite(baseMSG))
				{

				}

				producer->writeMaterialMessage(baseMSG, materialMSG);


				//Update Material Callback
				MObject obj = plugs[0].node();

				int index = getMaterialCallbackDataIndexByMesh(materialMSG.m_meshName);

				std::cout << "Index: " << index << std::endl;

				if (index != -1)
				{
					std::cout << "Callback Removed for " << matCallbackData[index].m_materialName << std::endl;

					strcpy_s(matCallbackData[index].m_meshName, materialMSG.m_meshName);
					strcpy_s(matCallbackData[index].m_materialName, lShader.name().asChar());

					//Remove old callback
					MMessage::removeCallback(matCallbackData[index].m_callbackID);

					if (res == MStatus::kSuccess)
					{
						std::cout << "Callback removed succesfully" << std::endl;
					}

					//Add new callback
					matCallbackData[index].m_callbackID = MNodeMessage::addAttributeChangedCallback(obj, attributeChangedMaterial, NULL, &status);

					std::cout << "Callback Added for " << matCallbackData[index].m_materialName << std::endl;

					if (materialMSG.m_shadingType == ViewerMessage::SHADING_TYPE::TEXTURED)
					{
						MObject fileTextureNode = connectedPlugs[0].node();

						if (matCallbackData[index].hasTextureFileCallback != -1)
						{
							MMessage::removeCallback(matCallbackData[index].m_textureFileCallbackID);
							matCallbackData[index].hasTextureFileCallback = -1;
						}

						std::cout << "File Callback Added" << std::endl;
						matCallbackData[index].m_textureFileCallbackID = MNodeMessage::addAttributeChangedCallback(fileTextureNode, attributeChangedFile, NULL, &status);
						matCallbackData[index].hasTextureFileCallback = 1;
					}
					else
					{
						if (matCallbackData[index].hasTextureFileCallback != -1)
						{
							MMessage::removeCallback(matCallbackData[index].m_textureFileCallbackID);
							matCallbackData[index].hasTextureFileCallback = -1;
						}
					}

				}
				else
				{
					MaterialCallbackData empty;
					strcpy_s(empty.m_meshName, materialMSG.m_meshName);
					strcpy_s(empty.m_materialName, lShader.name().asChar());

					if (materialMSG.m_shadingType == ViewerMessage::SHADING_TYPE::TEXTURED)
					{
						MObject fileTextureNode = connectedPlugs[0].node();

						std::cout << "File Callback Added" << std::endl;
						matCallbackData[index].m_textureFileCallbackID = MNodeMessage::addAttributeChangedCallback(fileTextureNode, attributeChangedFile, NULL, &status);
						matCallbackData[index].hasTextureFileCallback = 1;
					}

					//Add new callback
					empty.m_callbackID = MNodeMessage::addAttributeChangedCallback(obj, attributeChangedMaterial, NULL, &status);

					std::cout << "Callback Added for " << empty.m_materialName << std::endl;

					matCallbackData.push_back(empty);


				}

			}
		}

	}
}

void topologyModified(MObject& node, void* clientData)
{
	MStatus res;

	MFnMesh mesh(node, &res);

	if (res == MStatus::kSuccess)
	{
		std::vector<ViewerMessage::VertexMessage> vertexMessages;
		getMeshVertexdata(node, vertexMessages);

		ViewerMessage::BaseMessage baseMSG;
		ViewerMessage::TopologyChangedMessage topoChangeMSG;

		topoChangeMSG.m_nrOfVertices = vertexMessages.size();
		strcpy_s(topoChangeMSG.m_meshName, mesh.name().asChar());

		baseMSG.m_msgType = ViewerMessage::MSG_TYPE::TopologyChanged;
		baseMSG.m_messageSize = sizeof(ViewerMessage::TopologyChangedMessage) + (sizeof(ViewerMessage::VertexMessage) * vertexMessages.size());

		while (!producer->producerCanWrite(baseMSG))
		{

		}

		producer->writeTopologyChangedMessage(baseMSG, topoChangeMSG, vertexMessages);

		std::cout << "Wrote Topology Changed Message" << std::endl;
		std::cout << "Offset: " << producer->getOffset() << std::endl;
		std::cout << "Other Offset: " << producer->getOtherOffset() << std::endl;

	}
}

//NODE ADDED
void nodeAdded(MObject& node, void* clientData)
{
	MStatus res;
	MFnDependencyNode nodeFn(node, &res);
	
	if (node.hasFn(MFn::kTransform))
	{
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(node, attributeChangedTransform, NULL, &status));
	}

}

void attributeChangedTransform(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	MStatus res;

	if (msg & MNodeMessage::kAttributeSet)
	{

		if (plug.node().hasFn(MFn::kTransform))
		{
			//Transform Node Changed

			//Fn
			MFnTransform childT(plug.node(), &res);

			if (res == MStatus::kSuccess)
			{
				getChildMatrix(plug.node());
			}


		}
	}

}

void attributeChangedFile(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	MStatus res;

	MFnDependencyNode depNode(plug.node());

	if (msg & MNodeMessage::kAttributeSet)
	{
		if (plug == depNode.findPlug("fileTextureName", true, &res))
		{

			ViewerMessage::BaseMessage baseMSG;
			ViewerMessage::FilePathMessage filePathMSG;

			baseMSG.m_msgType = ViewerMessage::MSG_TYPE::FilePath;
			baseMSG.m_messageSize = sizeof(ViewerMessage::FilePathMessage);

			MString filePath;
			plug.getValue(filePath);
			strcpy_s(filePathMSG.m_diffTexturePath, filePath.asChar());

			//Find Corresponding Mesh
			MPlug outColor = depNode.findPlug("outColor", true, &res);

			if (res == MStatus::kSuccess)
			{

				MPlugArray connectedPlugs;
				outColor.connectedTo(connectedPlugs, false, true, &res);

				if (res == MStatus::kSuccess && connectedPlugs.length() > 0)
				{
					if (connectedPlugs[0].node().hasFn(MFn::kLambert))
					{
						MFnLambertShader lShader(connectedPlugs[0].node(), &res);

						MPlug lOutColor = lShader.findPlug("outColor", true, &res);

						if (res == MStatus::kSuccess)
						{

							MPlugArray connectedPlugs2;
							lOutColor.connectedTo(connectedPlugs2, false, true, &res);

							if (res == MStatus::kSuccess && connectedPlugs2.length() > 0)
							{
								if (connectedPlugs2[0].node().hasFn(MFn::kShadingEngine))
								{
									MFnDependencyNode dNode(connectedPlugs2[0].node());

									MPlug dagPlug = dNode.findPlug("dagSetMembers", true, &res);

									if (res == MStatus::kSuccess && dagPlug.isArray())
									{

										//Get Plug Children
										MPlug dagChild = dagPlug[0];

										if (res == MStatus::kSuccess)
										{
											MPlugArray connectedPlugs3;
											dagChild.connectedTo(connectedPlugs3, true, false, &res);

											if (res == MStatus::kSuccess && connectedPlugs3.length() > 0)
											{

												MFnDependencyNode temp(connectedPlugs3[0].node());

												if (connectedPlugs3[0].node().hasFn(MFn::kMesh))
												{
													MFnMesh meshNode(connectedPlugs3[0].node());

													strcpy_s(filePathMSG.m_meshName, meshNode.name().asChar());

													//Send Message

													while (!producer->producerCanWrite(baseMSG))
													{

													}

													producer->writeFilePathMessage(baseMSG, filePathMSG);

													std::cout << "Wrote FilePath Message" << std::endl;
													std::cout << "Offset: " << producer->getOffset() << std::endl;
													std::cout << "Other Offset: " << producer->getOtherOffset() << std::endl;
												}
											}
										}

									}
								}
							}
						}
					}
				}
			}
		}
	}
	else if (msg & MNodeMessage::kConnectionBroken)
	{

		MMessage::removeCallback(MMessage::currentCallbackId());
	}
}

void attributeChangedMaterial(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData)
{
	MStatus res;

	if (msg & MNodeMessage::kAttributeSet)
	{

		if (plug.node().hasFn(MFn::kLambert))
		{

			MFnLambertShader lShader(plug.node(), &res);

			if (res == MStatus::kSuccess)
			{

				ViewerMessage::BaseMessage baseMSG;
				ViewerMessage::MaterialMessage materialMSG;

				baseMSG.m_msgType = ViewerMessage::MSG_TYPE::Material;
				baseMSG.m_messageSize = sizeof(ViewerMessage::MaterialMessage);

				//Determine if textured or colored

				//Diffuse Texture

				MPlug colorPlug = lShader.findPlug("color", true, &res);

				MPlugArray connectedPlugs;
				colorPlug.connectedTo(connectedPlugs, true, false, &res);

				if (res == MStatus::kSuccess && connectedPlugs.length() > 0)
				{

					if (connectedPlugs[0].node().hasFn(MFn::kFileTexture))
					{

						materialMSG.m_shadingType = ViewerMessage::SHADING_TYPE::TEXTURED;

						MFnDependencyNode fileNode(connectedPlugs[0].node(), &res);

						MPlug filePathPlug = fileNode.findPlug("fileTextureName", true, &res);

						if (res == MStatus::kSuccess)
						{
							MString filePath;
							filePathPlug.getValue(filePath);

							strcpy_s(materialMSG.m_diffTexturePath, filePath.asChar());
						}

					}
					else
					{
						materialMSG.m_shadingType = ViewerMessage::SHADING_TYPE::COLORED;
					}
				}

				MColor currentColor;
				currentColor = lShader.ambientColor(&res);

				materialMSG.m_ambient[0] = currentColor.r;
				materialMSG.m_ambient[1] = currentColor.g;
				materialMSG.m_ambient[2] = currentColor.b;
				materialMSG.m_ambient[3] = currentColor.a;

				currentColor = lShader.color(&res);

				materialMSG.m_diffuse[0] = currentColor.r;
				materialMSG.m_diffuse[1] = currentColor.g;
				materialMSG.m_diffuse[2] = currentColor.b;

				currentColor = lShader.transparency(&res);

				materialMSG.m_diffuse[3] = 1 - currentColor.r;

				//Mesh Name
				char matName[50];
				strcpy_s(matName, lShader.name().asChar());
				std::vector<int> indices = getMaterialCallbackDataIndicesByMaterial(matName);

				for (int i = 0; i < indices.size(); i++)
				{
					strcpy_s(materialMSG.m_meshName, matCallbackData[indices[i]].m_meshName);

					//Send Messages

					while (!producer->producerCanWrite(baseMSG))
					{

					}

					producer->writeMaterialMessage(baseMSG, materialMSG);

					std::cout << "Wrote Material Message" << std::endl;
					std::cout << "Offset: " << producer->getOffset() << std::endl;
					std::cout << "Other Offset: " << producer->getOtherOffset() << std::endl;
				}

			}


		}

	}
	else if (msg & MNodeMessage::kOtherPlugSet)
	{

		MFnDependencyNode depNode(plug.node(), &res);

		if (plug == depNode.findPlug("color", true, &res))
		{
			MFnLambertShader lShader(plug.node(), &res);

			if (res == MStatus::kSuccess)
			{

				ViewerMessage::BaseMessage baseMSG;
				ViewerMessage::MaterialMessage materialMSG;

				baseMSG.m_msgType = ViewerMessage::MSG_TYPE::Material;
				baseMSG.m_messageSize = sizeof(ViewerMessage::MaterialMessage);

				//Determine if textured or colored

				//Diffuse Texture

				MPlug colorPlug = lShader.findPlug("color", true, &res);

				MPlugArray connectedPlugs;
				colorPlug.connectedTo(connectedPlugs, true, false, &res);

				if (res == MStatus::kSuccess && connectedPlugs.length() > 0)
				{

					if (connectedPlugs[0].node().hasFn(MFn::kFileTexture))
					{
						materialMSG.m_shadingType = ViewerMessage::SHADING_TYPE::TEXTURED;

						MFnDependencyNode fileNode(connectedPlugs[0].node(), &res);

						MPlug filePathPlug = fileNode.findPlug("fileTextureName", true, &res);

						if (res == MStatus::kSuccess)
						{
							MString filePath;
							filePathPlug.getValue(filePath);

							strcpy_s(materialMSG.m_diffTexturePath, filePath.asChar());

							//Add callback to filenode
							MObject obj = connectedPlugs[0].node();
							callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(obj, attributeChangedFile, NULL, &status));
						}

					}
					else
					{
						materialMSG.m_shadingType = ViewerMessage::SHADING_TYPE::COLORED;
					}
				}

				MColor currentColor;
				currentColor = lShader.ambientColor(&res);

				materialMSG.m_ambient[0] = currentColor.r;
				materialMSG.m_ambient[1] = currentColor.g;
				materialMSG.m_ambient[2] = currentColor.b;
				materialMSG.m_ambient[3] = currentColor.a;

				currentColor = lShader.color(&res);

				materialMSG.m_diffuse[0] = currentColor.r;
				materialMSG.m_diffuse[1] = currentColor.g;
				materialMSG.m_diffuse[2] = currentColor.b;

				currentColor = lShader.transparency(&res);

				materialMSG.m_diffuse[3] = 1 - currentColor.r;

				//Mesh Name
				char matName[50];
				strcpy_s(matName, lShader.name().asChar());
				std::vector<int> indices = getMaterialCallbackDataIndicesByMaterial(matName);

				for (int i = 0; i < indices.size(); i++)
				{
					strcpy_s(materialMSG.m_meshName, matCallbackData[indices[i]].m_meshName);

					//Send Messages

					while (!producer->producerCanWrite(baseMSG))
					{

					}

					producer->writeMaterialMessage(baseMSG, materialMSG);

					std::cout << "Wrote Material Message" << std::endl;
					std::cout << "Offset: " << producer->getOffset() << std::endl;
					std::cout << "Other Offset: " << producer->getOtherOffset() << std::endl;
				}

			}
		}
	}

}

void cameraChanged(const MString& str, void* clientData)
{
	MString command = "getPanel -wf"; //wf means "with focus"
	MString activePanel = MGlobal::executeCommandStringResult(command);

	if (strcmp(str.asChar(), activePanel.asChar()) == 0)
	{
		M3dView currentView = M3dView::active3dView();
		
		currentView.updateViewingParameters();

		MDagPath cameraDAG;
		currentView.getCamera(cameraDAG);

		MFnCamera camera(cameraDAG.node());

		MFnTransform camTrans(cameraDAG.transform());

		//MSG
		ViewerMessage::CameraMessage cameraMSG;
		ViewerMessage::CameraTransformMessage cameraTransformMSG;

		MMatrix matrix;
		matrix = camTrans.transformationMatrix();

		float dest[4][4];
		matrix.get(dest);

		//Get data

		int count = 0;

		for (int i = 0; i < 4; i++)
		{

			for (int j = 0; j < 4; j++)
			{
				cameraTransformMSG.m_matrix[count] = dest[i][j];
				count++;
			}

		}


		//Send Cam Transform

		cameraTransformMessages.push_back(cameraTransformMSG);

		//Projection

		MFloatMatrix projMatrix = camera.projectionMatrix();
		projMatrix.get(dest);

		if (camera.isOrtho())
		{
			cameraMSG.m_projectionType = ViewerMessage::PROJECTION_TYPE::ORTHOGRAPHIC;
		}
		else
		{
			cameraMSG.m_projectionType = ViewerMessage::PROJECTION_TYPE::PERSPECTIVE;
		}

		//Get data
		count = 0;

		for (int i = 0; i < 4; i++)
		{

			for (int j = 0; j < 4; j++)
			{
				cameraMSG.m_projectionMatrix[count] = dest[i][j];
				count++;
			}

		}

		cameraMessages.push_back(cameraMSG);

	}
}

//TIMER
void timerCallback(float elapsedTime, float lastTime, void* clientData)
{
	mTimer.endTimer();
	mElapsedTime += mTimer.elapsedTime();
	mTimer.beginTimer();

	//Check Transform Messages
	if (transformMessages.size() > 0)
	{
		ViewerMessage::BaseMessage baseMSG;

		baseMSG.m_messageSize = sizeof(ViewerMessage::TransformMessage);
		baseMSG.m_msgType = ViewerMessage::MSG_TYPE::Transform;

		for (int i = 0; i < transformMessages.size(); i++)
		{
			int latest = transformMessages[i].size() - 1;
			
			while (!producer->producerCanWrite(baseMSG))
			{

			}

			producer->writeTransformMessage(baseMSG, transformMessages[i][latest]);

			std::cout << "Wrote Transform Message" << std::endl;
			std::cout << "Offset: " << producer->getOffset() << std::endl;
			std::cout << "Other Offset: " << producer->getOtherOffset() << std::endl;

			transformMessages[i].clear();
		}

		//Clear all messages
		transformMessages.clear();
	}

	//Check Single Vertex Messages
	if (singleVertexMessages.size() > 0)
	{
		ViewerMessage::BaseMessage baseMSG;

		baseMSG.m_messageSize = sizeof(ViewerMessage::SingleVertexMessage);
		baseMSG.m_msgType = ViewerMessage::MSG_TYPE::SingleVertex;

		for (int i = 0; i < singleVertexMessages.size(); i++)
		{
			for (int j = 0; j < singleVertexMessages[i].size(); j++)
			{
				while (!producer->producerCanWrite(baseMSG))
				{

				}

				std::cout << "Wrote Single Vertex Message" << std::endl;
				std::cout << "Offset: " << producer->getOffset() << std::endl;
				std::cout << "Other Offset: " << producer->getOtherOffset() << std::endl;

				producer->writeSingleVertexMessage(baseMSG, singleVertexMessages[i][j]);
			}
			singleVertexMessages[i].clear();
		}

		singleVertexMessages.clear();
	}

	//Check Camera Transform Messages
	if (cameraTransformMessages.size() > 0)
	{
		ViewerMessage::BaseMessage baseMSG;

		baseMSG.m_messageSize = sizeof(ViewerMessage::CameraTransformMessage);
		baseMSG.m_msgType = ViewerMessage::MSG_TYPE::CameraTransform;

		int latest = cameraTransformMessages.size() - 1;

		while (!producer->producerCanWrite(baseMSG))
		{

		}

		producer->writeCameraTransformMessage(baseMSG, cameraTransformMessages[latest]);

		std::cout << "Wrote Camera Transform Message" << std::endl;
		std::cout << "Offset: " << producer->getOffset() << std::endl;
		std::cout << "Other Offset: " << producer->getOtherOffset() << std::endl;

		cameraTransformMessages.clear();
	}

	//Check Camera Messages
	if (cameraMessages.size() > 0)
	{
		ViewerMessage::BaseMessage baseMSG;

		baseMSG.m_messageSize = sizeof(ViewerMessage::CameraMessage);
		baseMSG.m_msgType = ViewerMessage::MSG_TYPE::Camera;

		int latest = cameraMessages.size() - 1;

		while (!producer->producerCanWrite(baseMSG))
		{

		}

		producer->writeCameraMessage(baseMSG, cameraMessages[latest]);

		std::cout << "Wrote Camera Message" << std::endl;
		std::cout << "Offset: " << producer->getOffset() << std::endl;
		std::cout << "Other Offset: " << producer->getOtherOffset() << std::endl;

		cameraMessages.clear();
	}
}

//PLUGIN ENTRY POINT
EXPORT MStatus initializePlugin(MObject obj) {

	MStatus res = MS::kSuccess;

	MFnPlugin myPlugin(obj, "level editor", "1.0", "Any", &res);

	if (MFAIL(res)) {
		CHECK_MSTATUS(res);
		return res;
	}

	 //Redirect cout
	std::cout.set_rdbuf(MStreamUtils::stdOutStream().rdbuf());
	std::cerr.set_rdbuf(MStreamUtils::stdErrorStream().rdbuf());
	std::cout << "Plugin loaded ===========================" << std::endl;

	//Producer Variables
	LPCSTR fileMapObject = "mainFileMap";
	DWORD totalMemSize = 200 * 1024;

	//Create Producer
	producer = new ComLib(fileMapObject, totalMemSize, ComEnum::TYPE::PRODUCER);

	//Register Callbacks
	callbackIdArray.append(MDagMessage::addChildAddedCallback(childAdded, NULL, &status));
	callbackIdArray.append(MDGMessage::addNodeAddedCallback(nodeAdded, "dependNode", NULL, &status));

	//Timer Callback
	mTimer.clear();
	mTimer.beginTimer();
	callbackIdArray.append(MTimerMessage::addTimerCallback(0.4, timerCallback, NULL, &status));


	//Camera
	MDagPath camera;

	status = M3dView::active3dView().getCamera(camera);

	callbackIdArray.append(MUiMessage::add3dViewPreRenderMsgCallback("modelPanel1", cameraChanged));
	callbackIdArray.append(MUiMessage::add3dViewPreRenderMsgCallback("modelPanel2", cameraChanged));
	callbackIdArray.append(MUiMessage::add3dViewPreRenderMsgCallback("modelPanel3", cameraChanged));
	callbackIdArray.append(MUiMessage::add3dViewPreRenderMsgCallback("modelPanel4", cameraChanged));

	return res;
}


//PLUGIN EXIT POINT
EXPORT MStatus uninitializePlugin(MObject obj) 
{
	MFnPlugin plugin(obj);

	mTimer.endTimer();

	std::cout << "Plugin unloaded =========================" << std::endl;

	//Delete Producer
	delete producer;

	//Remove Callbacks
	MMessage::removeCallbacks(callbackIdArray);

	for (int i = 0; i < matCallbackData.size(); i++)
	{
		MMessage::removeCallback(matCallbackData[i].m_callbackID);

		if (matCallbackData[i].hasTextureFileCallback != -1)
		{
			MMessage::removeCallback(matCallbackData[i].m_textureFileCallbackID);
		}
	}

	return MS::kSuccess;
}