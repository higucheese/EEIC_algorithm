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
	struct HashTable *next;
};
typedef struct HashTable HashTable;

void initialize_table(HashTable* table){
	int i;
	for(i = 0; i < HASHTABLE_NUM_MAX; i++) {
		table[i].count = 0;
		table[i].word[0] = '\0';
		table[i].next = NULL;
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

unsigned int hash_function(char* word){
	//http://stackoverflow.com/questions/7666509/hash-function-for-string
	unsigned int hashval = 5381;
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
	else if(strncmp(table_element->word, word, WORD_LENGTH_MAX) == 0){
		table_element->count++;
	}
	//the word is differnt from original one
	else{
		while(table_element->next != NULL){
			table_element = table_element->next;
			if(strncmp(table_element->word, word, WORD_LENGTH_MAX) == 0){
				table_element->count++;
				return;
			}
		}
		table_element->next = (HashTable*)malloc(sizeof(HashTable));
		table_element->next->count = 1;
		strncpy(table_element->next->word, word, WORD_LENGTH_MAX);
	}
}

HashTable* table_max_freqency(HashTable* table_element){
	HashTable* temp;
	if(table_element->next != NULL){
		temp = table_max_freqency(table_element->next);
		if(temp->count > table_element->count) return temp;
		else return table_element;
	}else return table_element;
}

int if_exist(HashTable* table_element, char* word){
	if(strncmp(table_element->word, word, WORD_LENGTH_MAX) == 0) return 1;
	if(table_element->next != NULL){
		return if_exist(table_element->next, word);
	}else return 0;
}

void print_freqency(HashTable table1[][HASHTABLE_NUM_MAX], HashTable table2[][HASHTABLE_NUM_MAX]){
	int i, j, k, l;
	HashTable f[SEARCHNUM], *temp;
	for(i = 0;i < SEARCHNUM;i++){
		f[i].count = 0;
	}
	for(j = 0;j < TABLE_NUM;j++){
		for(i = 0;i < HASHTABLE_NUM_MAX;i++){
			temp = table_max_freqency(&table1[j][i]);
			for(l = 0;l < SEARCHNUM;l++){
				if(f[l].count <= temp->count && if_exist(&table2[j][i], temp->word) == 0){
					for(k = 1;k < SEARCHNUM - l;k++){
						f[SEARCHNUM - k] = f[SEARCHNUM - (k + 1)];
					}
					f[l] = *temp;
					break;
				}
			}
		}
	}
	for(i = 0;i < SEARCHNUM;i++){
		printf("%d:%s\n", f[i].count, f[i].word);
	}
}

void hashtable_free(HashTable* table_element){
    if(table_element->next != NULL){
		hashtable_free(table_element->next);
		free(table_element->next);
	}
}

void print_hashtable(HashTable* table_element){
	printf("%d:%s|", table_element->count, table_element->word);
	if(table_element->next != NULL){
		print_hashtable(table_element->next);
	}
}

int main(void){
	int i, j, state = BEFORE, wordlength = 0;
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
				unsigned int hash_score = hash_function(word);
				if(state == BEFORE){
					word_counter(word, &table_before[hash_score / HASHTABLE_NUM_MAX][hash_score % HASHTABLE_NUM_MAX]);
				}else{
					word_counter(word, &table_after[hash_score / HASHTABLE_NUM_MAX][hash_score % HASHTABLE_NUM_MAX]);
				}
				wordlength = 0;
			}else{
				//do nothing
			}
			break;
		}
		counter++;
	}
	
	print_freqency(table_before, table_after);
	printf("-------------------\n");
	print_freqency(table_after, table_before);
	
#ifdef DEBUG
	for(j = 0;j < TABLE_NUM;j++){
		for(i = 0;i < HASHTABLE_NUM_MAX;i++){
			if(table_before[j][i].count > 0){
				printf("%5d ", i + j * TABLE_NUM);
				print_hashtable(&table_before[j][i]);
				printf("\n");
			}
		}
	}
	printf("state:%d\n", state);
#endif
	
	for(j = 0;j < TABLE_NUM;j++){
		for(i = 0;i < HASHTABLE_NUM_MAX;i++){
			hashtable_free(&table_before[j][i]);
			hashtable_free(&table_after[j][i]);
		}
	}
	return 0;
}
