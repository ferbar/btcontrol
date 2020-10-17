#include <Button2.h>
#include <vector>
#include "lokdef.h"

class GuiView {
public:
  static GuiView currGuiView;

  GuiView() {};
  virtual ~GuiView() {};
  virtual void loop();
  static void startGuiView(const GuiView &newGuiView);
  static void runLoop();
  virtual void close() {};
  virtual void init() {};
  const char * which() const { return "GuiView" ; };
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
  static bool havePasswordForSSID(const String &ssid);
};

class GuiViewConnect : public GuiView {
public:
  GuiViewConnect(const String &ssid) : ssid(ssid) {};
  void init();
  void close();
  void loop();
  const char * which() const { return "guiViewConnect"; };
private:
  int lastWifiStatus;
  String ssid;
};

class GuiViewControl : public GuiView {
public:
  GuiViewControl(IPAddress host, int port) : host(host), port(port) {};
  GuiViewControl() {};
  void init();
  void close();
protected:
  static int selectedAddrIndex;
  static int nLokdef;
  IPAddress host;
  int port;
};

class GuiViewContolLocoSelectLoco : public GuiViewControl {
public:
	GuiViewContolLocoSelectLoco() {};
	void init();
	void close();
	void loop();
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

