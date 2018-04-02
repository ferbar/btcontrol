#ifndef LOKDEF_H
#define LOKDEF_H

#define F_DEFAULT 0
#define F_DEC14 1
#define F_DEC28 2

#define MAX_NFUNC 32

struct func_t {
	char name[32];
	bool pulse; // nur einmal einschalten dann gleich wieder aus
	bool ison;
	char imgname[32];
};

struct lokdef_t {
	int addr;
	int flags;		// LGB loks mit 14 fahrstufen ansteuern
	char name[20];
	int nFunc;
	func_t func[MAX_NFUNC]; // 16+1 0=licht, 1=F1, 2=F2
	int currspeed;
	int currdir;
	bool initDone;
	char imgname[32];
	int lastClientID;
};

// darf kein pointer auf const sein weil .currspeed und func wird ja geändert
extern lokdef_t *lokdef; 
/*= {
//	{1,"lok1"},
//	{2,"lok2"},
//	{3,"lok3",4,{{"func1"},{"func2"},{"func3"},{"func4"}}},
// die funk 5-12 werden extra übertragen -> sollten nicht stören
	{3,  F_DEFAULT,"lok3",12,{{"func1"},{"func2"},{"func3"},{"func4"},{"func5"},{"func6"},{"func7"},{"func8"},{"func9"},{"func10"},{"func11"},{"func12"}}},
	{4,  F_DEFAULT,"Ge 4/4 I",12,{{"sPfeife",true},{"sBremse",true},{"sPfeife+Echo"},{"sAnsage"},{"sAggregat ein/aus",false},{"sSound ein/aus",false},{"lFührerstand"},{"Rangiergang"},{"Ansage",true},
		{"sLufthahn"},{"Pantho"},{"Pantho"},{"sKompressor"},{"sVakuumpumpe"},{"sStufenschalter"},{"schaltbare Verzögerung"}}},
	{5,  F_DEFAULT,"lok5"},
	{6,  F_DEFAULT,"Ge 6/6 II",8,{{"sSound ein/aus"},{"sPfeife"},{"sRangierpfiff"},{"sKompressor"},{"sPressluft"},{"Rangiergang"}}},
	{413,F_DEFAULT,"Ge 6/6 I",12,{{"lFührerstand"},{"2"},{"3"},{"lFührerstand"},{"sPfeife",true},{"s6"},{"s7"},{"sSound ein/aus"},{"sPfeife"},{"sRangierpfiff"},{"sKompressor"},{"sPressluft"}}},
	{108,F_DEFAULT,"G 4/5",12,{{"lFührerstand"},{"2"},{"3"},{"lFührerstand"},{"sPfeife",true},{"s6"},{"s7"},{"sSound ein/aus"},{"sPfeife"},{"sRangierpfiff"},{"sKompressor"},{"sPressluft"}}},
	{7,  F_DEFAULT,"Ge 4/4 II",8,{{"lFührerstand"},{"lFührerstand"},{"lSchweizer Rücklicht"},{"???"}}},
	{647,F_DEFAULT,"Ge 4/4 III",12,{{"lFührerstand"},{"2"},{"3"},{"lFührerstand"},{"sPfeife",true},{"s6"},{"s7"},{"sSound ein/aus"},{"sPfeife"},{"sRangierpfiff"},{"sKompressor"},{"sPressluft"}}},
	{203,F_DEFAULT,"Ge 2/6 203",12,{{"lFührerstand"},{"2"},{"3"},{"lFührerstand"},{"sPfeife",true},{"s6"},{"s7"},{"sSound ein/aus"},{"sPfeife"},{"sRangierpfiff"},{"sKompressor"},{"sPressluft"}}},
	{12, F_LGB_DEC,"Ge 2/4 213"},
	{14, F_DEFAULT,"schoema",4,{{""},{"lblink"},{"lblink"},{""}}},
	{0}
};
*/


int getAddrIndex(int addr);

bool readLokdef();

void dumpLokdef();

#endif
