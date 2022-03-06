// !!! set LONGCLICK_MS to at least 800 [ms]
#include <Button2.h>
#include <vector>
#include "lokdef.h"
#include "utils.h"
#include "Thread.h"

class GuiView {
public:
  static GuiView *currGuiView;

  GuiView() {};
  virtual ~GuiView() {};
  virtual void loop();
  // aufrufen mit new xyz !!! this-> wird dann ungültig !!!
  static void startGuiView(GuiView *newGuiView);
  static void runLoop();
  virtual void close() {};
  virtual void init() {};
  virtual const char * which() const { return "GuiView" ; };
protected:
  static long lastKeyPressed;         // power off after POWER_DOWN_IDLE_TIMEOUT, init mit 0 => system boot
  void drawButtons();
};

class GuiViewSelectWifi : public GuiView {
public:
  struct wifiConfigEntry_t {
    const char* ssid;
    const char* password;
  };

  GuiViewSelectWifi() { };
  void init();
  void close();
  void loop();
  static void buttonCallback(Button2 &b, int which);
  static void buttonCallbackLongPress(Button2 &b);
  const char * which() const { return "GuiViewSelectWifi"; };
  static bool needUpdate;
private:
  static int selectedWifi; // muss wegen callback static sein
  static const char *passwordForSSID(const String &ssid);
  int lastFoundWifis=0;
};

class GuiViewConnectWifi : public GuiView {
public:
  GuiViewConnectWifi(const String &ssid, const char *password, bool LR) : ssid(ssid), password(password), LR(LR), connectingStartedAt(millis()) {};
  void init();
  void close();
  void loop();
  const char * which() const { return "guiViewConnectWifi"; };
private:
  void displaySelectServer();
  int lastWifiStatus;
  String ssid;
  const char *password;
  bool LR;
  long connectingStartedAt;
  static bool needUpdate;
  static int mdnsResults;
  static int selectedServer; // muss wegen callback static sein
};

class GuiViewConnectServer : public GuiView {
public:
  GuiViewConnectServer(IPAddress host, int port) { this->host=host; this->port=port; };
  GuiViewConnectServer() {};
  void init();
  void close();
  void loop();
  // void loop(); - default
  const char * which() const { return "GuiViewConnectServer"; };
protected:
  static int nLokdef;
  static IPAddress host;
  static int port;
  int abortConnect=0;
};

class GuiViewControlLocoSelectLoco : public GuiView { // GuiViewConnectServer ???
public:
	GuiViewControlLocoSelectLoco() {needUpdate=true; };
	void init();
	void close();
	void loop();
	const char * which() const { return "GuiViewControlLocoSelectLoco"; };

	static bool needUpdate; // einfacher wegen callbacks wenn public
  static int firstLocoDisplayed;
};

class GuiViewControlLoco : public GuiView { // GuiViewConnectServer ???
public:
	GuiViewControlLoco() {};
	void init();
	void close();
	void loop();
	const char * which() const { return "GuiViewControlLoco"; };
	static void onClick(Button2 &b);
  void refreshBatLevel();
private:
	void sendSpeed(int what);
	static bool forceStop;              // Speed schieber muss auf 0 geschoben werden, bis dahin 'stop'
  int batLevelAddr=-1;
  int batLevelCV=-1;
  int batLevel=-1;
  unsigned long lastBatLevelRefresh=(unsigned) -30000;
};

class GuiViewErrorMessage : public GuiView {
public:
	GuiViewErrorMessage(const String &errormessage) : errormessage(errormessage), needUpdate(true) { };
	void init();
	void close() {};
	void loop();
	const char * which() const { return "GuiViewErrorMessage"; };
private:
	String errormessage;
	bool needUpdate;
	static int retries;
};

class GuiViewPowerDown : public GuiView {
public:
	GuiViewPowerDown() : startTime(millis()), done(false) {};
	void init();
	void close();
	void loop();
	const char * which() const { return "GuiViewPowerDown"; };
private:
	long startTime;
	bool done;
};

void resetButtons();
