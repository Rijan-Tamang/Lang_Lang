PROJECT NAME: Lang-Lang
Main kura chai k ho bhanda kheri, hamro afnai programming language banaunu lako jasko syntax chai Nepali ma hunxa jasto:
printf(“hello world”);
C ma ho vane
Dhekha(“hello world”); 
Chai hamro language ma hune vayo.
Aba tyo keywords jasto c ma hunxa ni testai hamro language hune haru chai sochnu parxa k k rakhda thik hola bhanera.
Yo kasari banaune ta tesko lagi hami sanga euta roadmap jasto xa 
 <img width="909" height="419" alt="image" src="https://github.com/user-attachments/assets/eccde30b-42b7-4d45-b8a3-8b59cc564790" />

Source code chai abha hamile lekheko file ma hunxa
For ex:
File name : helloworld.ll (ll chai hamro extension -> hellowold.c jastai)
File bhitra chai: 
	Dekha(“hello  world”);
Xare aba yo file lai compile garda chai terminal ma athawa result chai hello world aaunu paryo
Aba yo mathi ko road map xa ni tei nai follow garxam hami.
“compiler” banunu ko lagi chai.
Main three steps hunxa:
1. Lexing/tokenizing/scanning : mathi ko jun source code lai chai scan garera telai tokenize garinxa yestari:
[dekha] [(] [“hello world”] [)] [;]
2. parsing : yo tokenization bhako lai chai meaning dine process bhayo.
dekha predefined function ho jasle (“”) bhitra ko print garxa
3. code generate: tyo parsing paxi code generate hunxa jasle chai c++ ko code ma convert garxa
Yeti chai basic idea vayo aru chai afu pani research gar na ani kei idea aayo bhane hami lai pani bhanana
