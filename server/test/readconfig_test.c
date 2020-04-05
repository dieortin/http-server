#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "../readconfig.h"

/**
 * @var configuration
 * This global variable contains the structure responsible for the hash table.
 */
struct config_param *configuration = NULL;


int main() {
	printf("Maximum parameter name is %i\n", MAX_PARAM_NAME);
	config_addparam_int("Int1", 21);
	assert(config_getparam_int_n("Int1") == 21);
	assert(config_getparam_int_n("Int2") == -1);
	config_addparam_str("Str1", "Hello");
	assert(config_getparam_int_n("Str1") == -1);
	assert(config_getparam_str("Str2") == NULL);
	assert(strcmp(config_getparam_str("Str1"), "Hello") == 0);
	assert(config_getparam_str("Int1") == NULL);
	assert(config_getparam_str("Int2") == NULL);

	printf("Test correct!\n");
}
