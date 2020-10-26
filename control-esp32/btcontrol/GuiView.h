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
};



class GuiViewSelectWifi : public GuiView {
public:
  struct wifiConfigEntry_t {
    const char* ssid;
    const char* password;
  };
  struct wifiEntry_t {
    String ssid;
    long rssi;
  };

  GuiViewSelectWifi() { };
  void init();
  void close();
  void loop();
  static void buttonCallback(Button2 &b, int which);
  static void buttonCallbackLongPress(Button2 &b);
  const char * which() const { return "GuiViewSelectWifi"; };
private:
  static std::vector <wifiEntry_t> wifiList;
  static int selectedWifi;
  static bool needUpdate;
  static const char *passwordForSSID(const String &ssid);
};

class GuiViewConnect : public GuiView {
public:
  GuiViewConnect(const String &ssid, const char *password) : ssid(ssid), password(password) {};
  void init();
  void close();
  void loop();
  const char * which() const { return "guiViewConnect"; };
private:
  int lastWifiStatus;
  String ssid;
  const char *password;
};

class GuiViewControl : public GuiView {
public:
  GuiViewControl(IPAddress host, int port) { this->host=host; this->port=port; };
  GuiViewControl() {};
  void init();
  void close();
  // void loop(); - default
  const char * which() const { return "GuiViewControl"; };
protected:
  static int selectedAddrIndex;
  static int nLokdef;
  static IPAddress host;
  static int port;
};

class GuiViewContolLocoSelectLoco : public GuiViewControl {
public:
	GuiViewContolLocoSelectLoco() {needUpdate=true; };
	void init();
	void close();
	void loop();
	const char * which() const { return "GuiViewContolLocoSelectLoco"; };
private:
	static bool needUpdate;
};

class GuiViewControlLoco : public GuiViewControl {
public:
	GuiViewControlLoco() {};
	void init();
	void close();
	void loop();
	const char * which() const { return "GuiViewControlLoco"; };
	static void onClick(Button2 &b);
private:
	void sendSpeed(int what);
	static bool forceStop;
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
	void loop();
	const char * which() const { return "GuiViewPowerDown"; };
private:
	long startTime;
	bool done;
};

