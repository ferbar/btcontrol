// !!! set LONGCLICK_MS to at least 800 [ms]
#include <Button2.h>
#include <vector>
#include "lokdef.h"
#include "utils.h"
#include "Thread.h"
#include "config.h"
#ifdef HAVE_BLUETOOTH
#include "BTClient.h"
#endif

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
  static void drawPopup(const String &msg);
protected:
  static long lastKeyPressed;         // power off after POWER_DOWN_IDLE_TIMEOUT, init mit 0 => system boot
  void drawButtons();
  void drawButtons(const char *top1, const char *top2, const char *bottom1, const char *bottom2);
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

/**
 * select Wifi / BT Device
 */
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
  GuiViewConnectServer(IPAddress host, int port) { this->host=host; this->port=port; 
#ifdef HAVE_BLUETOOTH
    this->btAddr=BTAddress();
#endif
  };
#ifdef HAVE_BLUETOOTH
  GuiViewConnectServer(const BTAddress &addr, int channel) { this->btAddr=addr; this->channel=channel; this->host=IPAddress(); };
#endif
  GuiViewConnectServer() {};
  void init();
  void close();
  void loop();
  // void loop(); - default
  const char * which() const { return "GuiViewConnectServer"; };
  static bool canRetryConnect();
protected:
  static int nLokdef;
  static IPAddress host;
  static int port;
  int abortConnect=0;
#ifdef HAVE_BLUETOOTH
  static BTAddress btAddr;
  static int channel;
#endif
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
	GuiViewPowerDown(GuiView *viewIfButtonPressed) : startTime(millis()), done(false) {
    GuiViewPowerDown::viewIfButtonPressed=viewIfButtonPressed;
  };
	void init();
	void close();
	void loop();
	const char * which() const { return "GuiViewPowerDown"; };
  GuiView *viewIfButtonPressed=NULL;
private:
	long startTime;
	bool done;
};

void resetButtons();

class GuiViewInfo : public GuiView {
public:
	GuiViewInfo()  { };
	void init();
	void close();
	void loop();
	const char * which() const { return "GuiViewInfo"; };
  int page=0;
  bool forceRefresh=false;
};

