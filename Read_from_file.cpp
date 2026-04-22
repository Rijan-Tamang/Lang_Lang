#include <iostream>
#include <fstream>
using namespace std;
string file_name;
string store_content;
void read_file()
{
	store_content = "";
	fstream file("test_file.lang", ios::in);
	string line;
	while(getline(file, line))
		store_content += line;
}
int main(){
	file_name = "test_file.lang";
	read_file();
	cout << store_content << endl;
	return 0;
}
