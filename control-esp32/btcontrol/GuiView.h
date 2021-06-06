// !!! set LONGCLICK_MS to at least 800 [ms]
#include <Button2.h>
#include <vector>
#include "lokdef.h"
#include "utils.h"

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
private:
  static std::map <String, long> wifiList;
  static int selectedWifi; // muss wegen callback static sein
  static bool needUpdate;
  static const char *passwordForSSID(const String &ssid);
};

class GuiViewConnectWifi : public GuiView {
public:
  GuiViewConnectWifi(const String &ssid, const char *password) : ssid(ssid), password(password) {};
  void init();
  void close();
  void loop();
  const char * which() const { return "guiViewConnectWifi"; };
private:
  void displaySelectServer();
  int lastWifiStatus;
  String ssid;
  const char *password;
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
};

class GuiViewContolLocoSelectLoco : public GuiView { // GuiViewConnectServer ???
public:
	GuiViewContolLocoSelectLoco() {needUpdate=true; };
	void init();
	void close();
	void loop();
	const char * which() const { return "GuiViewControlLocoSelectLoco"; };
private:
	static bool needUpdate;
};

class GuiViewControlLoco : public GuiView { // GuiViewConnectServer ???
public:
	GuiViewControlLoco() {};
	void init();
	void close();
	void loop();
	const char * which() const { return "GuiViewControlLoco"; };
	static void onClick(Button2 &b);
private:
	void sendSpeed(int what);
	static bool forceStop;              // Speed schieber muss auf 0 geschoben werden, bis dahin 'stop'
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
