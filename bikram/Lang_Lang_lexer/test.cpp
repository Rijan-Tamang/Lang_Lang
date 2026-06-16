#include <iostream>
#include <string>
#include "globals.h"
extern string store_content;
int main(int argc, char* argv[]){
	main_read_from_file(argc, argv);
	int i;
	cout << "test.cpp" << endl;

	for (i=0; i < store_content.size(); i++){
		if(store_content[i] == '\n')
			cout << "new line" << i << endl;

	}


	return 0;
}
