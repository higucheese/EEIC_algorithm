//#define DEBUG

#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

#define HASHTABLE_NUM_MAX 1024
#define TABLE_NUM 64
#define WORD_LENGTH_MAX 32
#define BEFORE 0
#define AFTER 1
#define SEARCHNUM 5

struct HashTable{
	int count;
	char word[WORD_LENGTH_MAX];
};
typedef struct HashTable HashTable;

void initialize_table(HashTable* table){
	int i;
	for(i = 0; i < HASHTABLE_NUM_MAX; i++) {
		table[i].count = 0;
		table[i].word[0] = '\0';
	}
}

int character_identify(char *c){
	if('a' <= *c && *c <= 'z') return 0;
	else if('A' <= *c && *c <= 'Z') return 1;
    else if('0' <= *c && *c <= '9') return 0;
	else if(*c == '-' || *c == '\'') return 2;
	else if(*c == '@') return 3;
	else return 4;
}

unsigned int hash_function(char* word, unsigned int* delay){
	//http://stackoverflow.com/questions/7666509/hash-function-for-string
	unsigned int hashval = 5381 + *delay;
	while(*word != '\0'){
		//hashval = hashval * 33 + *word
		hashval = (hashval << 5) + hashval + (unsigned int)*word++;
	}
	return (hashval % (HASHTABLE_NUM_MAX * TABLE_NUM));
}

void word_counter(char* word, HashTable* table_element){
	//the word came at first
	if(table_element->count == 0) {
		strncpy(table_element->word, word, WORD_LENGTH_MAX);
		table_element->count++;
	}
	//the word is already known
	else{
		table_element->count++;
	}
}

void freq_ranking(HashTable* table1_element, HashTable table2[][HASHTABLE_NUM_MAX], HashTable* f, unsigned int* maxdelay){
	int k, l;
	unsigned int delay = 0, hash_score;
	for(l = 0;l < SEARCHNUM;l++){
		if(f[l].count <= table1_element->count){
			for(delay = 0;delay <= *maxdelay ; delay++){
				hash_score = hash_function(table1_element->word, &delay);
				if(strncmp(table1_element->word, table2[hash_score / HASHTABLE_NUM_MAX][hash_score % HASHTABLE_NUM_MAX].word, WORD_LENGTH_MAX) == 0){
					return;
				}
			}
			for(k = 1;k < SEARCHNUM - l;k++) f[SEARCHNUM - k] = f[SEARCHNUM - (k + 1)];
			f[l] = *table1_element;
			return;
		}
	}
}

void print_freqency(HashTable table1[][HASHTABLE_NUM_MAX], HashTable table2[][HASHTABLE_NUM_MAX], unsigned int* maxdelay){
	int i, j;
	HashTable f[SEARCHNUM];
	for(i = 0;i < SEARCHNUM;i++){
		f[i].count = 0;
	}
	for(j = 0;j < TABLE_NUM;j++){
		for(i = 0;i < HASHTABLE_NUM_MAX;i++){
			if(f[SEARCHNUM-1].count < table1[j][i].count){
				freq_ranking(&table1[j][i], table2, f, maxdelay);
			}
		}
	}
	for(i = 0;i < SEARCHNUM;i++){
		printf("%d:%s\n", f[i].count, f[i].word);
	}
}

int main(void){
	int i, state = BEFORE, wordlength = 0;
	unsigned int maxdelay = 0;
	char *c, word[WORD_LENGTH_MAX];
	HashTable table_before[TABLE_NUM][HASHTABLE_NUM_MAX], table_after[TABLE_NUM][HASHTABLE_NUM_MAX];
	for(i = 0;i < TABLE_NUM;i++){
		initialize_table(table_before[i]);
		initialize_table(table_after[i]);
	}

	int filesize, pagesize, mmapsize;
	filesize = (int)lseek(0, 0, SEEK_END);
	pagesize = getpagesize();
	mmapsize = (filesize + (pagesize - 1)) / pagesize * pagesize;
	c = (char*)mmap(0, (size_t)mmapsize, PROT_READ, MAP_PRIVATE, 0, 0);
	if(c == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
	int counter = 0;
	while(counter < filesize){
		switch(character_identify(&c[counter])){
		case 0:
			word[wordlength] = c[counter];
			if(wordlength < WORD_LENGTH_MAX - 1) wordlength++;
			break;
		case 1:
			word[wordlength] = (char)((int)c[counter] + 'a' - 'A');
			if(wordlength < WORD_LENGTH_MAX - 1) wordlength++;
			break;
		case 2:
			if(wordlength > 0){
				word[wordlength] = c[counter];
				if(wordlength < WORD_LENGTH_MAX - 1) wordlength++;
			}else{
				//do nothing
			}
			break;
		case 3:
			state = AFTER;
		default:
			if(wordlength > 0){
				if(word[wordlength - 1] == '\'' || word[wordlength - 1] == '-'){
					word[wordlength - 1] = '\0';
				}else{
					word[wordlength] = '\0';
				}
				unsigned int hash_score, access1, access2, delay = 0;
				while(1){
					hash_score = hash_function(word, &delay);
					access1 = hash_score / HASHTABLE_NUM_MAX;
					access2 = hash_score % HASHTABLE_NUM_MAX;
					if(state == BEFORE){
						if(table_before[access1][access2].count == 0 || strncmp(word, table_before[access1][access2].word, WORD_LENGTH_MAX) == 0) break;
					}else {
						if(table_after[access1][access2].count == 0 || strncmp(word, table_after[access1][access2].word, WORD_LENGTH_MAX) == 0) break;
					}
					delay++;
				}
				if(delay > maxdelay) maxdelay = delay;
				if(state == BEFORE){
					word_counter(word, &table_before[access1][access2]);
				}else{
					word_counter(word, &table_after[access1][access2]);
				}
				wordlength = 0;
			}else{
				//do nothing
			}
			break;
		}
		counter++;
	}

	print_freqency(table_before, table_after, &maxdelay);
	printf("-------------------\n");
	print_freqency(table_after, table_before, &maxdelay);

#ifdef DEBUG
	int j;
	for(j = 0;j < TABLE_NUM;j++){
		for(i = 0;i < HASHTABLE_NUM_MAX;i++){
			if(table_before[j][i].count > 0){
				printf("%5d %d:%s\n", i + j * HASHTABLE_NUM_MAX, table_before[j][i].count, table_before[j][i].word);
			}
		}
	}
	printf("------------------\n");
    for(j = 0;j < TABLE_NUM;j++){
		for(i = 0;i < HASHTABLE_NUM_MAX;i++){
			if(table_after[j][i].count > 0){
				printf("%5d %d:%s\n", i + j * HASHTABLE_NUM_MAX, table_after[j][i].count, table_after[j][i].word);
			}
		}
	}

	printf("maxdelay:%d\n", maxdelay);
#endif
	return 0;
}
