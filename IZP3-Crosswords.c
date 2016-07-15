#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

const char* P_HELPMSG =
	"SolveIt v0.1\n"
	"Author: Jan Spisiak xspisi03@stud.fit.vutbr.cz\n"
	"Program for solving word finding game.\n\n"
	"  Usage:\n"
	"    ./proj3 [-h] (--test,--search=%word%,--solve) sigdig [(double)a]\n"
	"  Options:\n"
	"    -h\n"
	"        This help message.\n\n"
	"    --test %charMatrixFile%\n"
	"        Tests whether the file is valid charMatrix file.\n"
	"    --search=%word% %charMatrixFile%\n"
	"        Searches for word and highlights it.\n"
	"    --solve %charMatrixFile% %wordsFile%\n"
	"        Solves given charMatrixFile with wordsFile.\n";

/*
 * Error codes enum.
 */
typedef enum _enErrc {
	EOK,
	EMALLOC,
	EFORMAT,
	EFILE,
	ERANGE,
	ENOTFOUND,
	errcSize
} errc;

/*
 * Error codes strings.
 */
const char* _pacErrcStr[] = {
	[EMALLOC] = "EMALLOC: Problem allocating memory.",
	[EFORMAT] = "EFORMAT: Input is of wrong format.",
	[EFILE] = "EFILE: Couldn't open file.",
	[ERANGE] = "ERANGE: Wrong range.",
	[ENOTFOUND] = "ENOTFOUND: Not found."
};

/*
 * Error code to string.
 */
const char* errcToStr( errc er ) {
	if (er < errcSize) {
		return _pacErrcStr[er];
	} else {
		return NULL;
	}
}


/*
 * All possible chars we can store in wordTree
 */
//a b c d e f g h i j k l m n o p q r s t u v w x y z ch
#define	WORD_TREE_ALLOWED_CHARS_SIZE	27

/*
 * Typedef of struct _stWordTreeNode so we can use it in our struct definition.
 */
typedef struct _stWordTreeNode wordTreeNode;

/*
 * Struct for wordTreeNode
 */
struct _stWordTreeNode {
	// How many words have their route through this node.
	unsigned int routes;
	// Pointer to array of pointers to nodes.
	wordTreeNode** nodes;
	// Frequency of word.
	unsigned int freq;
};

/*
 * Root of wordTree.
 */
typedef struct _stWordTree {
	unsigned int routes;
	wordTreeNode** nodes;
	// Length of the tree (ie. longest path).
	unsigned int length;
} wordTree;

/*
 * Frees the whole node.
 */
errc _wordTree_destroyNode( wordTreeNode** ppwtn ) {
	free((*ppwtn)->nodes);
	free(*ppwtn);
	*ppwtn = NULL;
	return EOK;
}

/*
 * Allocates and creates new node. (does not allocate node array)
 */
errc _wordTree_newNode( wordTreeNode** ppwtn ) {
	if ((*ppwtn = malloc(sizeof(wordTreeNode))) == NULL) {
		return EMALLOC;
	}
	wordTreeNode* pwtn = *ppwtn;
	pwtn->routes = 0;
	pwtn->nodes = NULL;
	pwtn->freq = 0;
	return EOK;
}

/*
 * This function iterates through tree and frees any allocated nodes.
 */
errc wordTree_free( wordTree* wt ) {
	errc er = EOK;
	int i = 0;
	wordTreeNode* route[wt->length];
	unsigned int pos[wt->length];
	memset(pos,0,(wt->length) * sizeof(unsigned int));
	wordTreeNode* pwtn = (wordTreeNode*)wt;

	route[0] = (wordTreeNode*)wt;
	int j;
	// Main loop that iterates through all nodes
	while ( i >= 0 ) {
		pwtn = route[i];
		if (pwtn->nodes == NULL) {
			// Just a simple node, we dont have to iterate over its nodes.
			free(pwtn);
			i--;
			continue;
		}
		for ( j = pos[i]; j < WORD_TREE_ALLOWED_CHARS_SIZE; j++ ) {
			if ( pwtn->nodes[j] == NULL ) {
				continue;
			}
			// we found not NULL node so we iterate over it
			pos[i] = j + 1;
			route[i+1] = pwtn->nodes[j];
			pos[i+1] = 0;
			i++;
			break;
		}
		if ( j == WORD_TREE_ALLOWED_CHARS_SIZE ) {
			// if all nodes were checked we free the array and then free the node (unless its root node)
			free(pwtn->nodes);
			if (i > 0) {
				free(pwtn);
			}
			i--;
		}
	}
	return er;
}

/*
 * Gets pointer to node and sets it to next node using char.
 */
errc wordTree_iterateNode( wordTreeNode** ppwtn, const char ch ) {
	unsigned char nch = ch - 'a';
	wordTreeNode* pwtn = *ppwtn;
	if (nch > WORD_TREE_ALLOWED_CHARS_SIZE) {
		return EFORMAT;
	}
	if ((pwtn->nodes == NULL) || (pwtn->nodes[nch] == NULL )) {
		return ENOTFOUND;
	}
	*ppwtn = pwtn->nodes[nch];
	return EOK;
}

/*
 * Adds a new word int word tree.
 */
errc wordTree_addWord( wordTree* wt, const char* word ) {
	errc er = EOK;
	int i = 0;
	unsigned int wordSize = strlen(word);
	if (wt->length < wordSize ) {
		wt->length = wordSize;
	}
	wordTreeNode* route[wt->length + 1];
	wordTreeNode* pwtn = (wordTreeNode*)wt;
	wordTreeNode* pnwtn = NULL;
	unsigned char nch;
	while ( word[i] != '\0' ) {
		nch = word[i] - 'a';
		if (nch > WORD_TREE_ALLOWED_CHARS_SIZE) {
			er = EFORMAT;
			break;
		}
		// Check if we need to alloc the array.
		if (pwtn->nodes == NULL) {
			pwtn->nodes = malloc(sizeof(wordTreeNode*) * WORD_TREE_ALLOWED_CHARS_SIZE);
			if (pwtn->nodes == NULL) {
				er = EMALLOC;
				break;
			}
			for ( int i = 0; i < WORD_TREE_ALLOWED_CHARS_SIZE; i++ ) {
				pwtn->nodes[i] = NULL;
			}
		}
		if ((pnwtn = pwtn->nodes[nch]) == NULL ) {
			if ((er = _wordTree_newNode(&pnwtn)) != EOK) {
				break;
			}
			pwtn->nodes[nch] = pnwtn;
		}
		pwtn->routes++;
		route[i] = pwtn;
		pwtn = pnwtn;
		i++;
	}
	route[i] = pwtn;
	if ( i == 0 ) {
		return er;
	}
	// Something went wrong, delete garbage.
	if ( er != EOK ) {
		for ( i--; i >= 0; i-- ) {
			pwtn = route[i];
			if ( pwtn->routes > 1 ) {
				pwtn->routes--;
			} else {
				_wordTree_destroyNode(&pwtn);
			}
		}
		return er;
	}
	(route[i])->freq++;
	return EOK;
}

/*
 * Transform the word so that it changes to valid chars.
 */
errc wordTree_transformWord( char* word, unsigned int wordMaxSize ) {
	unsigned int i = 0;
	while( i < wordMaxSize ) {
		if (word[i] == '\0') {
			break;
		}
		if ((word[i] == 'c') && (word[i+1] == 'h')) {
			word[i] = 'a' + 26;
			memmove(&(word[i+1]),&(word[i+2]),(wordMaxSize - i)*sizeof(char));
		}
		i++;
	}
	return EOK;
}

/*
 * Reads one word.
 */
errc wordTree_readWord( FILE* fh, const char* format, char* word, unsigned int wordMaxSize ) {
	if (fscanf(fh,format,word) == EOF) {
		// This is ok.
		return ERANGE;
	}
	if (word[wordMaxSize] != '\0') {
		return EFORMAT;
	}
	wordTree_transformWord(word, wordMaxSize);
	return EOK;
}

errc wordTree_readFile( wordTree* wt, const char* filename, unsigned int wordMaxSize ) {
	errc er = EOK;
	FILE* fh;
	if ((fh = fopen(filename, "r")) == NULL) {
		return EFILE;
	}
	// Our buffer that holds temporarily the string.
	char* chBuffer = malloc( (wordMaxSize + 2) * sizeof(*chBuffer) );
	if ( chBuffer == NULL ) {
		er = EMALLOC;
	} else {
		memset(chBuffer,'\0',wordMaxSize+2);
		char format[16];
		sprintf(format,"%%%ds",wordMaxSize+1);
		while ((er = wordTree_readWord(fh, format, chBuffer, wordMaxSize )) == EOK) {
			wordTree_addWord(wt,chBuffer);
		}
		if (er == ERANGE) {
			er = EOK;
		}
	}
	fclose(fh);
	free(chBuffer);
	return er;
}



/*
 * Struct for the charMatrix using single dimension array.
 */
typedef struct _stCharMatrix {
	unsigned int rows, cols;
	char* table;
} charMatrix;

/*
 * Free internal allocations of charMatrix.
 */
errc charMatrix_free( charMatrix *pcm ) {
	pcm->rows = 0;
	pcm->cols = 0;
	free(pcm->table);
	pcm->table = NULL;
	return EOK;
}

/*
 * Initialize allocations for charMatrix.
 */
errc charMatrix_init( charMatrix* pcm, unsigned int rows, unsigned int cols ) {
	assert(pcm != NULL);
	pcm->table = malloc( rows * cols * sizeof(*(pcm->table)));
	if (pcm->table == NULL) {
		pcm->rows = 0;
		pcm->cols = 0;
		return EMALLOC;
	}
	pcm->rows = rows;
	pcm->cols = cols;
	return EOK;
}

/*
 * Converts char to lowercase.
 */
char charMatrix_tolower( char ch ) {
	if ( (ch >= 'A') && (ch <= 'Z' + 1) ) {
		ch += 32;
	}
	return ch;
}

/*
 * Checks whether char is lowercase.
 */
char charMatrix_islower( char ch ) {
	if ( (ch >= 'a') && (ch <= 'z' + 1) ) {
		return ch;
	}
	return 0;
}

/*
 * Converts char to uppercase.
 */
char charMatrix_toupper( char ch ) {
	if ( (ch >= 'a') && (ch <= 'z' + 1) ) {
		ch -= 32;
	}
	return ch;
}

/*
 * Properly reads 1 element of charMatrix and sets the value of char *pch to correct value.
 */
errc charMatrix_readChar( FILE* fh, char* pch ) {
	char charBuffer[4] = {'\0'};
	int rfscanf;
	if ((rfscanf = fscanf(fh,"%3s",charBuffer)) != 1) {
		return EFORMAT;
	}
	charBuffer[0] = tolower(charBuffer[0]);
	charBuffer[1] = tolower(charBuffer[1]);
	if (charBuffer[1] != '\0') {
		if (charBuffer[2] != '\0') {
			return EFORMAT;
		}
		if (strncmp("ch",charBuffer,2) == 0) {
			*pch = 'a' + 26;
			return EOK;
		} else {
			return EFORMAT;
		}
	} else {
		if ((charBuffer[0] <= 122) && (charBuffer[0] >= 97)) {
			*pch = charBuffer[0];
			return EOK;
		} else {
			return EFORMAT;
		}
	}
}

/*
 * Reads the file and loads it into empty charMatrix.
 */
errc charMatrix_readFile( charMatrix* pcm, const char* filename ) {
	FILE* fh;
	errc er = EOK;
	if ((fh = fopen(filename, "r")) == NULL) {
		return EFILE;
	}
	unsigned int r,s;
	if (fscanf(fh,"%u %u",&r,&s) != 2) {
		er = EFORMAT;
	} else {
		int length = r * s;
		if ((er = charMatrix_init(pcm, r, s)) == EOK ) {
			for ( int i = 0; i < length; i++ ) {
				if ((er = charMatrix_readChar(fh, &(((char*)pcm->table)[i]))) != EOK ) {
					break;
				}
			}
		}
	}
	fclose(fh);
	return er;
}

/*
 * Prints out the whole structure of charMatrix.
 */
errc charMatrix_print( charMatrix* pcm ) {
	printf("%u %u\n",pcm->rows,pcm->cols);
	for (unsigned int i = 0; i < pcm->rows * pcm->cols; i++) {
		if ( pcm->table[i] == 'a' + 26 ) {
			printf("ch ");
		} else if ( pcm->table[i] == 'A' + 26 ) {
			printf("CH ");
		} else {
			printf("%2c ",pcm->table[i]);
		}
		if ((i + 1) % pcm->cols == 0) {
			printf("\n");
		}
	}
	return EOK;
}

/*
 * Prints only lowercase chars from charMatrix.
 */
errc charMatrix_printTableLower( charMatrix* pcm ) {
	for (unsigned int i = 0; i < pcm->rows * pcm->cols; i++) {
		if ( charMatrix_islower( pcm->table[i] ) ) {
			printf("%c",pcm->table[i]);
		}
	}
	printf("\n");
	return EOK;
}

/*
 * All possible directions in 2D matrix.
 */
const int matrixDirection[][2] = {
	{0, 1},
	{1, 1},
	{1, 0},
	{1, -1},
	{0, -1},
	{-1, -1},
	{-1, 0},
	{-1, 1}
};

/*
 * Solves charMatrix with given wordTree.
 */
errc charMatrix_solve( charMatrix* pcm, wordTree* wt ) {
	wordTreeNode* pwtn;
	wordTreeNode* lastPwtn;
	errc er;
	int x,y,vx,vy;
	unsigned int it;
	for ( unsigned int i = 0; i < pcm->cols * pcm->rows; i++ ) {
		for ( unsigned int j = 0; j < 8; j++ ) {
			pwtn = (wordTreeNode*)wt;
			lastPwtn = NULL;
			x = i % pcm->cols;
			y = i / pcm->cols;
			vx = matrixDirection[j][0];
			vy = matrixDirection[j][1];
			it = 0;
			while ((er = wordTree_iterateNode(&pwtn, charMatrix_tolower(pcm->table[y*pcm->cols + x]))) == EOK) {
				if (!((x + vx >= 0) && (x + vx < (int)pcm->cols)))
					break;
				if (!((y + vy >= 0) && (y + vy < (int)pcm->rows)))
					break;

				if ((i % pcm->cols == 2) && (i / pcm->cols == 1)) {
					//printf("vx %d vy %d c %c %d\n",vx,vy,tolower(pcm->table[y*pcm->cols + x]),pwtn->freq);
				}
				if ( pwtn->freq > 0 ) {
					//printf("Word1 on %d %d\n",x,y);
					lastPwtn = pwtn;
				}
				x += vx;
				y += vy;
				it++;
			}
			if (( pwtn->freq > 0) && (er == EOK)) {
				//printf("Word2 on %d %d\n",x,y);
				lastPwtn = pwtn;
				x += vx;
				y += vy;
				it++;
			}
			if ( lastPwtn != NULL ) {
				//printf("Found length %d from %d %d to %d %d\n", it, x - it*vx, y - it*vy,x,y);
				for ( ; it > 0; it-- ) {
					//printf("it %d  x %d y %d\n",it,x,y);
					x -= vx;
					y -= vy;
					pcm->table[y*pcm->cols + x] = charMatrix_toupper(pcm->table[y*pcm->cols + x]);
				}
			}
		}
	}
	return EOK;
}


// Verbosity levels
typedef enum {
	PLL_ERROR,
	PLL_WARNING,
	PLL_NOTICE,
	PLL_MSG
} p_log_level;

// Prefix of logging entries
const char* P_LOG_LEVEL_MSGS[] =
{
	[PLL_ERROR] = 	"E: ",
	[PLL_WARNING] = "W: ",
	[PLL_NOTICE] = 	"N: ",
	[PLL_MSG] = 	"",
};

// Logging function
void p_log( p_log_level ll, const char* msg, ... ) {
	va_list args;
	va_start( args, msg );
	fprintf( stderr, "%s", P_LOG_LEVEL_MSGS[ll] );
	vfprintf( stderr, msg, args );
	fprintf( stderr, "\n");
	va_end( args );
}

typedef enum {
	pm_help,
	pm_test,
	pm_search,
	pm_solve
} p_modes;

struct p_arguments {
	p_modes mode;
	const char* searchWord;
	const char* wordsFilename;
	const char* charMatrixFilename;
};

errc p_resolveArguments(int argc, char** argv, struct p_arguments* pa) {
	if (( argc < 2 ) || (strcmp(argv[1],"-h") == 0)) {
		pa->mode = pm_help;
		return EOK;
	}

	if (argc < 3) {
		p_log(PLL_ERROR,"Not enough arguments.");
		return EFORMAT;
	}

	if (strcmp(argv[1],"--test") == 0) {
		pa->mode = pm_test;
		if (argc < 3)
			return EFORMAT;
		pa->charMatrixFilename = argv[2];
		return EOK;
	} else if (strcmp(argv[1],"--solve") == 0) {
		pa->mode = pm_solve;
		if (argc < 4)
			return EFORMAT;
		pa->charMatrixFilename = argv[2];
		pa->wordsFilename = argv[3];
		return EOK;
	} else if (strncmp(argv[1],"--search=",9) == 0) {
		pa->mode = pm_search;
		pa->searchWord = &(argv[1][9]);
		if (argc < 3)
			return EFORMAT;
		pa->charMatrixFilename = argv[2];
		return EOK;
	} else {
		p_log(PLL_ERROR,"Wrong argument \"%s\".",argv[1]);
		return EFORMAT;
	}
	return EFORMAT;
}


int main( int argc, char** argv )
{
	struct p_arguments pa;
	errc er;
	pa.charMatrixFilename = NULL;
	pa.searchWord = NULL;
	pa.wordsFilename = NULL;
	er = p_resolveArguments(argc, argv, &pa);
	if (er == EFORMAT) {
		// Wrong arguments
		p_log(PLL_ERROR,"Wrong argument format. Please see -h");
		return EXIT_SUCCESS;
	}
	if ( pa.mode == pm_help ) {
		p_log(PLL_MSG,"%s",P_HELPMSG);
		return EXIT_SUCCESS;
	} else if ( pa.mode == pm_test ) {
		// Test the charMatrix file.
		charMatrix cm;
		if ( (er = charMatrix_readFile(&cm, pa.charMatrixFilename)) != EOK ) {
			p_log(PLL_ERROR,"%d %s\n",er,errcToStr(er));
		} else {
			charMatrix_print(&cm);
		}
		charMatrix_free(&cm);
	} else if ( pa.mode == pm_search ) {
		// Search for word.
		charMatrix cm;
		wordTree wt;
		wt.length = 0;
		wt.nodes = NULL;
		wt.routes = 0;
		if ( (er = charMatrix_readFile(&cm, pa.charMatrixFilename)) != EOK ) {
			p_log(PLL_ERROR,"%d %s\n",er,errcToStr(er));
		} else {
			unsigned int searchWordLen = strlen(pa.searchWord);
			char word[searchWordLen + 1];
			strcpy(word, pa.searchWord);
			wordTree_transformWord(word, searchWordLen);
			if ( (er = wordTree_addWord(&wt, word)) != EOK ) {
				p_log(PLL_ERROR,"%d %s\n",er,errcToStr(er));
			} else {
				charMatrix_solve(&cm, &wt);
				charMatrix_print(&cm);
			}
		}
		charMatrix_free(&cm);
		wordTree_free(&wt);
	} else if ( pa.mode == pm_solve ) {
		// Solve
		charMatrix cm;
		wordTree wt;
		wt.length = 0;
		wt.nodes = NULL;
		wt.routes = 0;
		if ( (er = charMatrix_readFile(&cm, pa.charMatrixFilename)) != EOK ) {
			p_log(PLL_ERROR,"%d %s\n",er,errcToStr(er));
		} else {
			// compute the wordMaxSize so that we can use appropriate buffer.
			unsigned int wordMaxSize;
			if (cm.rows > cm.cols) {
				wordMaxSize = 2*cm.rows;
			} else {
				wordMaxSize = 2*cm.cols;
			}
			if ( (er = wordTree_readFile(&wt, pa.wordsFilename, wordMaxSize)) != EOK ) {
				p_log(PLL_ERROR,"%d %s\n",er,errcToStr(er));
			} else {
				charMatrix_solve(&cm, &wt);
				charMatrix_printTableLower(&cm);
			}
		}
		charMatrix_free(&cm);
		wordTree_free(&wt);
	}
    return EXIT_SUCCESS;
}
