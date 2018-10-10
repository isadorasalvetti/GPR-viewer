#include <iostream>
#include "trianglemesh.h"


using namespace std;

int next(int corner)
{
	return 3 * (corner / 3) + (corner + 1) % 3;
}

int previous(int corner)
{
	return 3 * (corner / 3) + (corner + 2) % 3;
}


TriangleMesh::TriangleMesh() : vboVertices(QOpenGLBuffer::VertexBuffer),
										 vboNormals(QOpenGLBuffer::VertexBuffer),
										 eboTriangles(QOpenGLBuffer::IndexBuffer)
{
}


void TriangleMesh::addVertex(const QVector3D &position)
{
	vertices.push_back(position);
}

void TriangleMesh::addTriangle(int v0, int v1, int v2)
{
	triangles.push_back(v0);
	triangles.push_back(v1);
	triangles.push_back(v2);
}

void TriangleMesh::buildCube()
{
	GLfloat vertices[] = {-1, -1, -1,
                          1, -1, -1,
                          1,  1, -1,
                         -1,  1, -1,
                         -1, -1,  1,
                          1, -1,  1,
                          1,  1,  1,
                         -1,  1,  1
                        };

	GLuint faces[] = {3, 1, 0, 3, 2, 1,
                    5, 6, 7, 4, 5, 7,
                    7, 3, 0, 0, 4, 7,
                    1, 2, 6, 6, 5, 1,
                    0, 1, 4, 5, 4, 1,
                    2, 3, 7, 7, 6, 2
                  };

	int i;

	for(i=0; i<8; i+=1)
		addVertex(0.5f * QVector3D(vertices[3*i], vertices[3*i+1], vertices[3*i+2]));
	for(i=0; i<12; i++)
		addTriangle(faces[3*i], faces[3*i+1], faces[3*i+2]);
}

bool TriangleMesh::init(QOpenGLShaderProgram *program)
{
	vector<QVector3D> replicatedVertices, normals;
	vector<unsigned int> perFaceTriangles;

	buildReplicatedVertices(replicatedVertices, normals, perFaceTriangles);

    buildCornerTable();

	program->bind();

	vao.destroy();
	vao.create();
	if(vao.isCreated())
		vao.bind();
	else
		return false;

	vboVertices.destroy();
	vboVertices.create();
	if(vboVertices.isCreated())
		vboVertices.bind();
	else
		return false;
	vboVertices.setUsagePattern(QOpenGLBuffer::StaticDraw);
	program->enableAttributeArray(0);
	program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);

	vboNormals.destroy();
	vboNormals.create();
	if(vboNormals.isCreated())
		vboNormals.bind();
	else
		return false;
	vboNormals.setUsagePattern(QOpenGLBuffer::StaticDraw);
	program->enableAttributeArray(1);
	program->setAttributeBuffer(1, GL_FLOAT, 0, 3, 0);

	eboTriangles.destroy();
	eboTriangles.create();
	if(eboTriangles.isCreated())
		eboTriangles.bind();
	else
		return false;
	eboTriangles.setUsagePattern(QOpenGLBuffer::StaticDraw);

	fillVBOs(replicatedVertices, normals, perFaceTriangles);

	vao.release();
	program->release();

	return true;
}

void TriangleMesh::destroy()
{
	vao.destroy();
	vboVertices.destroy();
	vboNormals.destroy();
	eboTriangles.destroy();

	vertices.clear();
	triangles.clear();
}

void TriangleMesh::render(QOpenGLFunctions &gl)
{
	vao.bind();
	eboTriangles.bind();
	gl.glDrawElements(GL_TRIANGLES, triangles.size(), GL_UNSIGNED_INT, 0);
	vao.release();
}

void TriangleMesh::buildReplicatedVertices(vector<QVector3D> &replicatedVertices, vector<QVector3D> &normals, vector<unsigned int> &perFaceTriangles)
{
	normals.resize(triangles.size());

	for(unsigned int i=0; i<triangles.size(); i+=3)
	{
		replicatedVertices.push_back(vertices[triangles[i]]);
		replicatedVertices.push_back(vertices[triangles[i+1]]);
		replicatedVertices.push_back(vertices[triangles[i+2]]);

		QVector3D N = QVector3D::crossProduct(vertices[triangles[i+1]] - vertices[triangles[i]], vertices[triangles[i+2]] - vertices[triangles[i]]);
		N.normalize();
		normals[i] = N;
		normals[i+1] = N;
		normals[i+2] = N;

		perFaceTriangles.push_back(perFaceTriangles.size());
		perFaceTriangles.push_back(perFaceTriangles.size());
		perFaceTriangles.push_back(perFaceTriangles.size());
	}
}

void TriangleMesh::fillVBOs(vector<QVector3D> &replicatedVertices, vector<QVector3D> &normals, vector<unsigned int> &perFaceTriangles)
{
	vboVertices.bind();
	vboVertices.allocate(&replicatedVertices[0], 3 * sizeof(float) * replicatedVertices.size());
	vboVertices.release();

	vboNormals.bind();
	vboNormals.allocate(&normals[0], 3 * sizeof(float) * normals.size());
	vboNormals.release();

	eboTriangles.bind();
	eboTriangles.allocate(&perFaceTriangles[0], sizeof(int) * perFaceTriangles.size());
	eboTriangles.release();
}


//Corner table calculation
void TriangleMesh::buildCornerTable(){

    /* Get/ store edge-corner data. (1)
     * If current edge has already been visited, remove the pair. (2)
     * Assign the pair as corners. (3)
    */

    int NoTris = triangles.size()/3;

    tableCornerEdges.resize(NoTris);
    tableCornerVertices.resize(vertices.size());

    for (int i = 0; i < NoTris; i++){ //(1)
            int v0 = triangles[i*3 + 0];
            int v1 = triangles[i*3 + 1];
            int v2 = triangles[i*3 + 2];
            CornerEdge edgeCorner; edgeCorner.corner = v0; edgeCorner.vertexA = v1; edgeCorner.vertexB = v2;
            tableCornerEdges[i] = edgeCorner;

            /*
            if (tableCornerEdges.size()==0) {tableCornerEdges[0] = edgeCorner);} //add the first pair.
            for (int k = 0; k < tableCornerEdges.size(); k++){
                //check if edge has been visited
                if (tableCornerEdges[k] == edgeCorner || tableCornerEdges[k].checkDouble(edgeCorner)) {
                    break;
                }
                //remove pair if not
                else if (k == tableCornerEdges.size()-1) {
                    tableCornerEdges[i] = (edgeCorner);
                }
            }
       }
      */

    }
    std::cout<<"Corner table generated"<<std::endl;
}

vector<int> TriangleMesh::GetVertexNeighboors(){
    vector<int> Neighboors;
    //for ()
}

void TriangleMesh::GaussianCurvature(){
    vector<float> perVertCurvature(vertices.size());
    //k = 2pi - sum(anglei) / area.
    for (int i = 0; i < vertices.size(); i++){
        vector<float> angle;
    }
}


