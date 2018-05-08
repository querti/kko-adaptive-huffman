/*
 * Autor: Lubomir Gallovic (xgallo03)
 * Datum: 16.2.2018
 * Soubor: ahed.c
 * Komentar:
 */ 

#include "ahed.h"

bitStream* stream;

//creates new tree
//root - changes this pointer to created root
int createTree (Tree** root) {

	*root = malloc(sizeof(Tree));

	if (*root == NULL)
		return AHEDFail;

	(*root)->value = NEWNODE;
	(*root)->weight = 0;
	(*root)->order = 512;

	(*root)->parent = NULL;
	(*root)->left = NULL;
	(*root)->right = NULL;

	return AHEDOK;
}

//changes values of weights of all relevant nodes in tree bottom to top
//makes neccesary changes if tree stops being Huffman
//change node - first node to be incremented 
// leafPointerArray[] - array of nodes
int updateTree(Tree* changeNode, Tree* leafPointerArray[]) {

	uint16_t tempOrder;
	Tree* tempParent;

	while (changeNode->parent != NULL) { //subsequently go up the tree 

		Tree* highestNode = changeNode;
		for (int i=0; i<=512; i++) {//search for node with same weight and highest order
			if (leafPointerArray[i] != NULL) {
				if (leafPointerArray[i]->weight == changeNode->weight && highestNode->order < leafPointerArray[i]->order)
					highestNode = leafPointerArray[i];
			}
		}

		if (highestNode != changeNode && highestNode != changeNode->parent && changeNode->parent != NULL) { //if highest is not changenode or parent, swap

			tempOrder = changeNode->order;
			changeNode->order = highestNode->order;
			highestNode->order = tempOrder;

			tempParent = changeNode->parent;

			//swap nodes
			if (changeNode->parent->left == changeNode) {
				changeNode->parent->left = highestNode;
			} else if (changeNode->parent->right == changeNode) {
				changeNode->parent->right = highestNode;
			} else {
				printf("Tree error");
				return AHEDFail;
			}

			if (highestNode->parent->left == highestNode) {
				highestNode->parent->left = changeNode;
			} else if (highestNode->parent->right == highestNode) {
				highestNode->parent->right = changeNode;
			} else {
				printf("Tree error");
				return AHEDFail;
			}

			changeNode->parent = highestNode->parent;
			highestNode->parent = tempParent;

		}

		changeNode->weight++;
		changeNode = changeNode->parent;
	}
	//increase weight of root node
	changeNode->weight++;
	return AHEDOK;
}

//in case of new character creates new node
//character - value of the character
//nodePointer - pointer to NEWNODE node
int createNewNode (int character, Tree* nodePointer, Tree* leafPointerArray[]) {

	Tree* charPointer = malloc(sizeof(Tree));
	Tree* newNodePointer = malloc(sizeof(Tree));

	if (charPointer == NULL || newNodePointer == NULL)
		return AHEDFail;

	nodePointer->value = UNDEFINED;
	nodePointer->left = newNodePointer;
	nodePointer->right = charPointer;
	nodePointer->weight = 1;

	charPointer->value = character;
	charPointer->order = nodePointer->order - 1;
	charPointer->weight = 1;
	charPointer->parent = nodePointer;
	charPointer->left = charPointer->right = NULL;

	newNodePointer->value = NEWNODE;
	newNodePointer->order = nodePointer->order - 2;
	newNodePointer->weight = 0;
	newNodePointer->parent = nodePointer;
	newNodePointer->left = charPointer->right = NULL;

	for (int i=257; i<=512; i++) { //find array space for internal node
		if (leafPointerArray[i] == NULL) {
			leafPointerArray[i] = leafPointerArray[NEWNODE];
			break;
		}
	}
	//add new locations to array
	leafPointerArray[character] = leafPointerArray[NEWNODE]->right;
	leafPointerArray[NEWNODE] = leafPointerArray[NEWNODE]->left;

	if (nodePointer->parent != NULL) {
		if (updateTree(nodePointer->parent, leafPointerArray) == AHEDFail) {
			return AHEDFail;
		}
	}

	return AHEDOK;
}


//destroys huffman tree
//node - root node
void clearTree(Tree* node) {

	Tree* left = node->left;
	Tree* right = node->right;

	free(node);

	if (left!=NULL)
		clearTree(left);
	if (right!=NULL)
		clearTree(right);
}

//writes encoded byte, only if filled, manages unwritten part, works for unchded char or tree node
// uncoded - write uncoded char? (for first occurence)
//charNode - if coded, gets code from tree 
//character - if coded prints
//outputFile - ...
//return value - 0 = byte not sent 1= byte sent
int sendEncode(bool uncoded, Tree* charNode, uint8_t character, FILE *outputFile, tAHED *ahed) {

	uint32_t bit = 0;

	if (uncoded) { //sending plain char

		uint8_t mask;
		uint32_t offset;
		for (int i = 7; i>=0; i--) { //iterate from highest to lowest bit in char
			mask = 1 << (i);
			bit = ((mask & character) == 0) ? false : true;

			if (bit) {
				offset = 1 << 31;
				offset = offset >> stream->position;
				stream->bytes = stream->bytes | offset;
				stream->position++;
			} else {
				stream->position++;
			}

		}

	} else { //sending tree route

		uint32_t treeRoute = 0;
		uint8_t bitCounter = 0;
		uint32_t bit = 0;

		while (charNode->parent != NULL) { //traverse tree

			if (charNode->parent->right == charNode) { //right == 1
				//bits are written RIGHT to LEFT (so theyd be in correct order)
				bit = 1 << (bitCounter);
				treeRoute = treeRoute | bit;
				bitCounter++;

			} else if (charNode->parent->left == charNode) { //left == 0
				bitCounter++;
			} else {
				printf("Tree error\n");
				return AHEDFail;
			}
			charNode = charNode->parent;
		}

		uint8_t move = 32 - bitCounter;
		treeRoute = treeRoute << move;
		//bits position = 1110 001 0 0000...
		uint32_t mask;
		uint32_t offset;

		for (int i = 31; i >= 32-bitCounter; i--) { //add tree route to buffer

			mask = 1 << (i);
			bit = ((mask & treeRoute) == 0) ? false : true;

			if (bit) {
				offset = 1 << 31;
				offset = offset >> stream->position;
				stream->bytes = stream->bytes | offset;
				stream->position++;
			} else {
				stream->position++;
			}
		}
	}

	uint32_t mask = 255 << 24;
	uint32_t bigByte = 0;
	uint8_t byte = 0;

	while (stream->position >= 8) { //if more than 8 bits in buffer, send

		bigByte = stream->bytes & mask;
		bigByte = bigByte >> 24;
		byte = bigByte;

		fprintf(outputFile, "%c", byte);
		ahed->codedSize++;

		stream->bytes = stream->bytes << 8;
		stream->position = stream->position - 8;
	}
	return AHEDOK;
}

/* Nazev:
 *   AHEDEncoding
 * Cinnost:
 *   Funkce koduje vstupni soubor do vystupniho souboru a porizuje zaznam o kodovani.
 * Parametry:
 *   ahed - zaznam o kodovani
 *   inputFile - vstupni soubor (nekodovany)
 *   outputFile - vystupn?soubor (kodovany)
 * Navratova hodnota: 
 *    0 - kodovani probehlo v poradku
 *    -1 - p?i kodovani nastala chyba
 */
int AHEDEncoding(tAHED *ahed, FILE *inputFile, FILE *outputFile)
{

	Tree* leafPointerArray[512+1] = {NULL}; //up to 256 char nodes, up to 256 internal nodes, 1 NEWNODE node
	Tree* huffTree = NULL;
	uint8_t character;
	stream = malloc(sizeof(bitStream));
	stream->bytes = 0;
	stream->position = 0;
	bool firstChar = true;

	ahed->uncodedSize = 0;
	ahed->codedSize = 0;

	if (createTree(&huffTree) == AHEDFail)
		return AHEDFail;

	leafPointerArray[NEWNODE] = huffTree;

	while (feof(inputFile) == 0) { //go over characters in file

		uint32_t untrunced = fgetc(inputFile);
		if (untrunced == 4294967295) {
			break;
		}

		character = untrunced;
		ahed->uncodedSize ++;

		if (leafPointerArray[character] == NULL) { //new character

			if (firstChar) { //if first character, dont write NEWNODE
				if(sendEncode(true, NULL, character, outputFile, ahed) == AHEDFail) {
					clearTree(huffTree);
					return AHEDFail;
				}
				firstChar = false;
			} else {
				if (sendEncode(false, leafPointerArray[NEWNODE], 0, outputFile, ahed) == AHEDFail) { //send NEWNODE
					clearTree(huffTree);
					return AHEDFail;
				}
				if (sendEncode(true, NULL, character, outputFile, ahed) == AHEDFail) {
					clearTree(huffTree);
					return AHEDFail;
				}
			}

			if (createNewNode(character, leafPointerArray[NEWNODE], leafPointerArray) == AHEDFail) {
				clearTree(huffTree);
				return AHEDFail;
			}

		} else { //not new char

			if (sendEncode(false, leafPointerArray[character], 0, outputFile, ahed) == AHEDFail) {
				clearTree(huffTree);
				return AHEDFail;
			}
			if (updateTree(leafPointerArray[character], leafPointerArray) == AHEDFail) {
				clearTree(huffTree);
				return AHEDFail;
			}
		}
	}

	if (sendEncode(false, leafPointerArray[NEWNODE], 0, outputFile, ahed) == AHEDFail) { // et the end of file, send NEWNODE
		clearTree(huffTree);
		return AHEDFail;
	}

	if (stream->position != 0) { //if some bits are left in buffer

		uint32_t bigByte = stream->bytes >> 24;
		uint8_t lastByte = bigByte;
		fprintf(outputFile, "%c", lastByte);
		ahed->codedSize++;
	}

	clearTree(huffTree);
	return AHEDOK;
}

/* Nazev:
 *   AHEDDecoding
 * Cinnost:
 *   Funkce dekoduje vstupni soubor do vystupniho souboru a porizuje zaznam o dekodovani.
 * Parametry:
 *   ahed - zaznam o dekodovani
 *   inputFile - vstupni soubor (kodovany)
 *   outputFile - vystupn?soubor (nekodovany)
 * Navratova hodnota: 
 *    0 - dekodovani probehlo v poradku
 *    -1 - p?i dekodovani nastala chyba
 */
int AHEDDecoding(tAHED *ahed, FILE *inputFile, FILE *outputFile)
{

	Tree* leafPointerArray[512+1] = {NULL}; //up to 256 char nodes, up to 256 internal nodes, 1 NEWNODE node
	Tree* huffTree = NULL;
	Tree* currentNode = NULL;
	uint8_t character;
	uint32_t addStream;
	stream = malloc(sizeof(stream));
	stream->bytes = 0;
	stream->position = 0;

	bool firstChar = true;
	bool previousNew = false;

	ahed->uncodedSize = 0;
	ahed->codedSize = 0;

	if (createTree(&huffTree) == AHEDFail)
		return AHEDFail;

	currentNode = huffTree;
	leafPointerArray[NEWNODE] = huffTree;

	while (true) { //iterate over file

		if (stream->position <= 24) { //read new char only if buffer isnt full
			character = fgetc(inputFile);
			if (feof(inputFile) == 0) {

				ahed->codedSize++;
				addStream = character << 24;
				addStream = addStream >> stream->position;
				stream->bytes = stream->bytes | addStream;
				stream->position = stream->position + 8;
			}	
		}

		if (feof(inputFile) != 0 && stream->position < 8) { //if whole file has been read and less than byte is on buffer, end
			break;
		}
		

		if (firstChar || previousNew) { //read plain char

			character = stream->bytes >> 24;
			fprintf(outputFile, "%c", character);
			ahed->uncodedSize++;

			if (createNewNode(character, leafPointerArray[NEWNODE], leafPointerArray) == AHEDFail) {
				clearTree(huffTree);
				return AHEDFail;
			}

			//update buffer
			stream->bytes = stream->bytes << 8;
			stream->position = stream->position - 8;
			firstChar = previousNew = false;

		} else { //not plain character, traverses tree

			uint32_t mask = 1 << 31;
			bool bit;
			while (true) { //continues until buffer is empty

				bit = ((mask & stream->bytes) == 0) ? false : true;

				if (bit) //1... go right
					currentNode = currentNode->right;
				
				else // 0.. left 
					currentNode = currentNode->left;
				
				if (currentNode->value != UNDEFINED) { // if leaf node
					if (currentNode->value == NEWNODE) { //if NEWNODE, set flag and end iteration

						previousNew = true;
						stream->bytes = stream->bytes << 1;
						stream->position--;
						currentNode = huffTree;
						break;
					}

					character = currentNode->value;
					ahed->uncodedSize++;
					fprintf(outputFile, "%c", character);

					currentNode = huffTree;
					if (updateTree(leafPointerArray[character], leafPointerArray) == AHEDFail) {
						clearTree(huffTree);
						return AHEDFail;
					}
				}

				stream->bytes = stream->bytes << 1;
				stream->position--;

				if (stream->position == 0) //if whole buffer has been read, stop traversing tree
					break;
			}
		}
	}

	if (previousNew == false) { //NEWNODE should be last item in coded text
		printf("incorrect end, before EOF wasnt NEWNODE\n");
		return AHEDFail;
	}

	clearTree(huffTree);
	return AHEDOK;
}



