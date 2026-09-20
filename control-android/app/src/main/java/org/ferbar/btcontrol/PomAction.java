/*
 *  This file is part of btcontrol
 *
 *  Copyright (C) Christian Ferbar
 *
 *  btcontrol is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  btcontrol is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with btcontrol.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Programming on the Main - Interface
 * ladet die decoder definition von jmri.org runter
 */

package org.ferbar.btcontrol;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.StringReader;
import java.io.StringWriter;
import java.io.UnsupportedEncodingException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.xml.namespace.NamespaceContext;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerConfigurationException;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.TransformerFactoryConfigurationError;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;

/*
import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.util.EntityUtils;
*/
import org.ferbar.btcontrol.ControlAction.AvailLocosListItem;
import org.ferbar.utils.utils;
import org.w3c.dom.Comment;
import org.w3c.dom.DOMException;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

import protocol.FBTCtlMessage;
import protocol.MessageLayouts;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.ListActivity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.HorizontalScrollView;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ScrollView;
import android.widget.TabHost;
import android.widget.TabHost.TabSpec;
import android.widget.TabHost.TabContentFactory;
import android.widget.TabWidget;
import android.widget.TextView;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.view.ViewGroup.LayoutParams;
import android.widget.Toast;

@TargetApi(8)
public class PomAction extends Activity {

	static String TAG = "btcontrol.PomGui";

	ArrayAdapter<AvailLocosListItemAddr> listAdapter = null;
	Object listAdapter_notify = new Object();
	private LayoutInflater mInflater;

	List<Integer> selectedLocosPos;

	static final String URL_ZIMO = "/xml/decoders/Zimo_Unified_software_MX690Sv30.xml";
	static final String URL_BLUETOOTHSERVER = "/xml/decoders/bluetoothserver.xml";
	static final String XincludeXMLns = "http://www.w3.org/2001/XInclude";
	String xmlHost;

	TabHost mTabHost;
	TextView tvInfo;
	static Document doc = null;
	// TODO: static Document docPane=null;

	static HashMap<String, View> tabViews = new HashMap<String, View>();
	// liste an pane nodes
	static ArrayList<Element> tabTabs = new ArrayList<Element>();

	static XPath xpath = XPathFactory.newInstance().newXPath();

	int manufacturer=-1;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.pom_neu);
		
		this.tvInfo = (TextView) findViewById(R.id.infoText);

		this.mTabHost = (TabHost) findViewById(android.R.id.tabhost);
		this.mTabHost.setup();
		this.selectedLocosPos = new ArrayList<Integer>();

		xpath.setNamespaceContext(new MyNamespaceContext());

		// JMRI decoder beschreibung laden:
		if (doc == null) {
			// Löscht den Cache für erfolgreiche DNS-Abfragen sofort (0 Sekunden TTL)
			java.security.Security.setProperty("networkaddress.cache.ttl", "0");
			// Löscht den Cache für FEHLGESCHLAGENE DNS-Abfragen sofort (wichtig für dich!)
			java.security.Security.setProperty("networkaddress.cache.negative.ttl", "0");

			// 20260920: workaround damit das android 4.1 keine ipv6 requests macht
			// Zwingt Java, nur noch IPv4-Sockets zu öffnen
			System.setProperty("java.net.preferIPv4Stack", "true");
			// Sagt Java, dass es IPv4-Adressen bei der Auflösung bevorzugen soll
			System.setProperty("java.net.preferIPv6Addresses", "false");




            final PomAction PA=this;
			Thread thread = new Thread(new Runnable() {
				@Override
				public void run() {
					try {
						// XMLParser parser = new XMLParser();
						String decoderurl;
						String partsXMLs[]={};
						if(manufacturer == 15) { // chris
							xmlHost = "http://" + AndroidMain.btcomm.getRemoteAddress();
							decoderurl = xmlHost + PomAction.URL_BLUETOOTHSERVER;
						} else {
							// ist von cloudflare gesperrt
							// host="http://jmri.org";
							xmlHost="https://raw.githubusercontent.com/JMRI/JMRI/refs/heads/master/";
							decoderurl = xmlHost + PomAction.URL_ZIMO;
							partsXMLs = new String[]{
									"/xml/programmers/parts/BasicPane.xml",
									"/xml/programmers/parts/MotorPane.xml",
									"/xml/programmers/parts/BasicSpeedControlPane.xml",
									"/xml/programmers/parts/SpeedTablePane.xml",
									"/xml/programmers/parts/FunctionMapPane.xml",
									"/xml/programmers/parts/LightsPane.xml",
									"/xml/programmers/parts/AnalogControlsPane.xml",
									"/xml/programmers/parts/ConsistPane.xml",
									"/xml/programmers/parts/AdvancedPane.xml",
									"/xml/programmers/parts/SoundPane.xml",
									"/xml/programmers/parts/SoundLevelsPane.xml",
									"/xml/programmers/parts/CVsPane.xml" };
						}

						{
							String xml = PomAction.getXmlFromUrl(decoderurl); // getting
							// XML
							PomAction.doc = PomAction.getDomElement(xml); // getting DOM element
							// includes aus Comprehensive.xml, an letztes child
							// anhängen

						}

						// NodeList nl = doc.getElementsByTagName(KEY_ITEM);

						Log.d("xml", doc.toString());


						replaceIncludes(PomAction.doc, 0, "root", PA);

						for (String path : partsXMLs) {
							Log.d(TAG,"loading url: "+xmlHost + path);
							String xml = PomAction.getXmlFromUrl(xmlHost + path); // getting
																// XML
							if(xml==null) {
								Log.w(TAG,"url "+ xmlHost+path + " didn't return a valid xml");
								continue;
							}
							Document subdoc = PomAction.getDomElement(xml); // getting
																			// DOM
																			// element
							NodeList list = subdoc.getChildNodes();
							Node newChild = null;
							if (list.item(0) instanceof Element)
								newChild = list.item(0);
							else if (list.item(1) instanceof Element)
								newChild = list.item(1);
							if (newChild != null) {
								Node tempDoc = doc.importNode(newChild,
										true);
								// lastParent.appendChild(tempDoc);
							}
						}
						// Log.i(tag,"============================= dumping generated xml ===========================");
						// dumpElement(doc);

						// PomAction.dumpElement((Node) doc);

						// panes raussuchen und dann die tabs anlegen:
						String expression = "//pane";
						NodeList nodes;
						nodes = (NodeList) xpath.evaluate(expression, doc,
								XPathConstants.NODESET);
						Log.d(TAG, nodes.toString());
						// NodeList nodes =
						// doc.getElementsByTagName("pane");
						if(nodes.getLength()==0) {
							throw new Exception("no panes found!");
						}
						for (int i = 0; i < nodes.getLength(); i++) {
							Node node = nodes.item(i);
							Log.d(TAG, node.toString());

							/*
							 * NamedNodeMap baseElmnt_gold_attr =
							 * node.getAttributes(); for (int j = 0; j <
							 * baseElmnt_gold_attr.getLength(); ++j) { Node
							 * attr = baseElmnt_gold_attr.item(j);
							 * Log.d(tag, attr.getNodeName() + " = \"" +
							 * attr.getNodeValue() + "\""); }
							 */

							if (node instanceof Element) {
								// Element child = (Element)node;
								// String name = child.getAttribute("name");
								// NodeList names = (NodeList)
								// xpath.evaluate("name", node,
								// XPathConstants.NODESET);
								Node nodeName = (Node) xpath.evaluate(
										"name", node, XPathConstants.NODE);
								/*
								 * if(names.getLength()==0) { Log.e(tag,
								 * "empty node name!"); } else { Element
								 * e=(Element) names.item(0); Log.d(tag,
								 * "Name element:" + e.toString());
								 */
								String name = nodeName.getTextContent();
								Log.i(TAG, "add tab " + name);
								PomAction.tabTabs.add((Element) node);
								// }

							}
						}

						PomAction.this.runOnUiThread(new Runnable() {
							public void run() {
								Log.d("UI thread", "I am the UI thread");
								for (Element node : PomAction.tabTabs) {
									PA.setupTab(node);
								}
								PA.tvInfo.setText("");
							}
						});
					} catch (Exception e) {
						Log.e(TAG, "Exception", e);
						PomAction.this.runOnUiThread(new Runnable() {
							public void run() {
								PA.tvInfo.setText("Error: "+e.getMessage());
							}
						});
						PomAction.doc=null;
					}
					
				}

			});
			Log.d(TAG,"asking for manufactorer ID");
			try {
				FBTCtlMessage msg = new FBTCtlMessage();
				msg.setType(MessageLayouts.messageTypeID("POM"));
				msg.get("addr").set(ControlAction.currSelectedAddr.get(0));
				msg.get("cv").set(BTcommThread.CV_MANUFACTURER);
				msg.get("value").set(-1);
				AndroidMain.btcomm.addCmdToQueue(msg,new BTcommThread.BtCommCallback() {
					public void BTCallback(FBTCtlMessage reply) {
						Log.d(TAG,"got manufacturer reply");
						try {
							manufacturer=reply.get("value").getIntVal();
							Log.d(TAG,"got manufacturer reply: "+manufacturer);
						} catch (Exception e) {
							Log.e(TAG,"failed", e);
						}
						thread.start();
					}
				} );
			} catch (Exception e) {
				Log.e(TAG,"query manufacturer failed", e);
				runOnUiThread(new Runnable() {
					@Override
					public void run() {
						Toast.makeText(getApplicationContext(), e.getMessage(), Toast.LENGTH_LONG).show();
						PA.tvInfo.setText("error: "+e.getMessage());
					}
				});
				throw new RuntimeException(e);
			}
		} else { // nur tabs anlegen:
			for (Element node : PomAction.tabTabs) {
				this.setupTab(node);
			}
		}

		// mInflater =
		// (LayoutInflater)getSystemService(Context.LAYOUT_INFLATER_SERVICE);

		// registerForContextMenu(getListView());
		/*
		 * Bundle bundle = getIntent().getExtras(); if(bundle != null) {
		 * this.selectMehrfachsteuerung
		 * =bundle.getBoolean("Mehrfachsteuerung",false); ImageButton ib =
		 * (ImageButton) this.findViewById(R.id.imageButtonStartMulti);
		 * this.orgStartMultiButtonImage = ib.getDrawable(); } else {
		 * this.findViewById(R.id.linearLayoutMulti).setVisibility(View.GONE); }
		 */

		// startFillData();
		// String[] mStrings = new String[]{"Android", "Google", "Eclipse"};
		// this.setListAdapter(new ArrayAdapter<String>(this,
		// android.R.layout.simple_list_item_1, mStrings));
	}
	
	public void dumpElement(final Node node) {
		try {
			StringWriter writer = new StringWriter();
			Transformer transformer;
			transformer = TransformerFactory.newInstance().newTransformer();
			// if (node instanceof Element) {
			transformer
					.transform(new DOMSource(node), new StreamResult(writer));
			String generatedxml = writer.toString();
			for (String line : generatedxml.split("\n")) {
				Log.d(TAG, line);
			}
			// Log.d(tag, generatedxml);
		} catch (TransformerConfigurationException e) {
			Log.e(TAG, "TransformerConfigurationException ", e);
			Toast.makeText(getApplicationContext(), e.getMessage(), Toast.LENGTH_LONG).show();
		} catch (TransformerFactoryConfigurationError e) {
			Log.e(TAG, "TransformerFactoryConfigurationError ", e);
			Toast.makeText(getApplicationContext(), e.getMessage(), Toast.LENGTH_LONG).show();
		} catch (TransformerException e) {
			Log.e(TAG, "TransformerException ", e);
			Toast.makeText(getApplicationContext(), e.getMessage(), Toast.LENGTH_LONG).show();
		}
	}

	/**
	 * legt ein neues tab an, tabhost muss inited sein + setup() aufgerufen
	 * 
	 */
	private void setupTab(final Element node) {
		Node nodeName;
		try {
			nodeName = (Node) xpath.evaluate("name", node, XPathConstants.NODE);
		} catch (XPathExpressionException e) {
			Log.e(TAG, "XPathExpressionException", e);
			Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
			return;
		}
		String tabName = nodeName.getTextContent();

		TabSpec tab = this.mTabHost.newTabSpec(tabName);
		// View tabview = createTabView(this.mTabHost.getContext(), tag);
		// newlines einbauen:
		Pattern p = Pattern
				.compile("^(\\w{3,})([\\s/-]+)(\\w{3,})(([\\s/-]+)(.*))?$");
		Matcher m = p.matcher(tabName);
		String tabIndicator = tabName;
		if (m.matches()) {
			tabIndicator = m.group(1) + "\n" + m.group(3);
			if (m.group(6) != null) {
				tabIndicator += "\n" + m.group(6);
			}
		}
		tab.setIndicator(tabIndicator); // kann auch ein view sein!

		tab.setContent(new TabContentFactory() {

			public View createTabContent(String tag) {
				/*
				 * if(PomAction.tabViews.containsKey(tag)) { // TODO: checken
				 * passt das mit dem parent - context return
				 * PomAction.tabViews.get(tag); }
				 */
				View tabView = PomAction.this.createPOMView(node);
				if (tabView != null) {
					PomAction.tabViews.put(tag, tabView);
				}
				return tabView;
			}

		});

		this.mTabHost.addTab(tab);
		// new line für tab Indicator einschalten:
		TabWidget tw = this.mTabHost.getTabWidget();
		TextView title = (TextView) tw.getChildAt(tw.getChildCount() - 1)
				.findViewById(android.R.id.title);
		title.setSingleLine(false);
	}

	/**
	 * legt eine view für den Tab Indicator an, unused
	 * 
	 * @param context
	 * @param text
	 * @return
	 *
	 *         private static View createTabView(final Context context, String
	 *         text) {
	 * 
	 *         LinearLayout view = new LinearLayout(context); / *
	 *         LayoutInflater.from(context) .inflate(R.layout.tabs_bg, null);
	 * 
	 *         TextView tv = (TextView) view.findViewById(R.id.tabsText); /
	 *         TextView tv = new TextView(context); // ~ alle 4 Zeichen ein
	 *         newline reinhaun:
	 *         text=text.replaceAll("^(\\w{3,})([\\s\\/]+)(.*)$","$1\n$3");
	 * 
	 *         tv.setText(text); view.addView(tv);
	 * 
	 *         return view;
	 * 
	 *         }
	 */

	/**
	 * legt das view dings für die Tab Seite an
	 *
	 * @return View
	 */
	private View createPOMView(Element node) {
		LinearLayout ret = new LinearLayout(this);
		ret.setOrientation(LinearLayout.VERTICAL);
		/*
		 * TextView tv = new TextView(this); try { Node nodeName = (Node)
		 * xpath.evaluate("name", node, XPathConstants.NODE); String
		 * tabName=nodeName.getTextContent(); tv.setText(tabName); } catch
		 * (XPathExpressionException e) {
		 * Log.e(tag,"XPathExpressionException",e); tv.setText("invalid"); }
		 * ret.addView(tv);
		 */

		try {
			Log.d(TAG, node.toString());
			ScrollView v = new ScrollView(this);
			v.setFillViewport(true);
			HorizontalScrollView hv = new HorizontalScrollView(this);
			hv.setFillViewport(true);
			LayoutParams params = new LayoutParams(LayoutParams.MATCH_PARENT,
					LayoutParams.MATCH_PARENT);
			v.setLayoutParams(params);
			hv.setLayoutParams(params);
			hv.addView(createPOMViewRecursive(node));
			v.addView(hv);
			ret.addView(v);

		} catch (XPathExpressionException e) {
			Log.e(TAG, "XPathExpressionException", e);
			Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
		}
		return ret;
	}

	private View createPOMViewRecursive(Element node)
			throws XPathExpressionException {
		View ret;
		String nodeName = node.getNodeName();
		Log.d(TAG, "createPOMViewRecursive: nodename=" + nodeName);
		if (nodeName.equals("column") || nodeName.equals("row")
				|| nodeName.equals("pane")
				|| nodeName.equals("grid") || nodeName.equals("griditem")) {
			Log.d(TAG, "add view recursive");
			LinearLayout ll_ret = new LinearLayout(this);
			if (nodeName.equals("column")) {
				// TODO: vertical setzen
				ll_ret.setOrientation(LinearLayout.VERTICAL);
			}
			for (Node childNode = (Node) node.getFirstChild(); childNode != null; childNode = (Node) childNode
					.getNextSibling()) {
				// Do something with childNode...
				if (childNode instanceof Element) {
					Element e = (Element) childNode;
					Log.d(TAG, "add view recursive ...");
					View v = createPOMViewRecursive(e);
					if (v != null) {
						ll_ret.addView(v);
					}
				}
			}
			ret = ll_ret;
		} else if (nodeName.equals("label")) {
			Element text = (Element) xpath.evaluate("text", node,
					XPathConstants.NODE);
			String label;
			if (text != null) {
				label = text.getTextContent();
			} else {
				label = "-----------";
			}
			Log.d(TAG, "add label [" + label + "]");
			TextView tv = new TextView(this);
			tv.setText(label);
			ret = tv;
		} else if (nodeName.equals("separator")) {
			Log.d(TAG, "add separator");
			View v = new View(this);
			LayoutParams params;
			if (node.getParentNode().getNodeName().equals("row")) {
				params = new LayoutParams(2, LayoutParams.MATCH_PARENT);
			} else {
				params = new LayoutParams(LayoutParams.MATCH_PARENT, 2);
			}
			v.setLayoutParams(params);
			int bgcolor = 0xFF909090;
			v.setBackgroundColor(bgcolor);
			ret = v;
		} else if (nodeName.equals("display")) {
			// PomAction.dumpElement((Element)
			// xpath.evaluate("/decoder-config/decoder/variables",
			// PomAction.doc, XPathConstants.NODE));
			String item = node.getAttribute("item");
			Log.d(TAG, "add display [" + item + "]");
			// String expression="//variable[@label='"+item+"']";
			// die xmls haben einen Bug: variables -> include und dort ist das root tag wieder variables ...
			String expression = "//variables/variable[@item='"
					+ item + "']";
			// --- da muss man rausfinden ob a) voller xpath funzt, b) @item=
			// oder @label=
			// variable label="Acceleration" CV="3" default="12" item="Accel"
			Element variableNode = (Element) xpath.evaluate(expression, PomAction.doc, XPathConstants.NODE);
			if (variableNode != null) {
				String format = variableNode.getAttribute("format");
				String cv = variableNode.getAttribute("CV");
				String label = "";
				Element labelNode = (Element) xpath.evaluate("label",  variableNode, XPathConstants.NODE);
				if(labelNode != null) {
					label = labelNode.getTextContent();
				} else if(variableNode.hasAttribute("label") ){
					label = variableNode.getAttribute("label");
				}
				String tooltip=null;
				if(variableNode.hasAttribute("tooltip")) {
					tooltip=variableNode.getAttribute("tooltip");
				}
				// zur liste hinzufügen:
				LinearLayout ll_ret = new LinearLayout(this);
				TextView tvLabel = new TextView(this);
				tvLabel.setText(label + " (#"	+ cv + ")");
				tvLabel.setOnClickListener(view -> {
					PomDialog.show(this, cv);
				});
				ll_ret.addView(tvLabel);
				if(tooltip != null) {
					TextView comment = new TextView(this);
					comment.setText(tooltip);
					ll_ret.addView(comment);
				}
				ret = ll_ret;
			} else {
				Log.e(TAG, "error finding " + expression);
				TextView comment = new TextView(this);
				comment.setText("didn't find [" + item + "]");
				ret = comment;
			}
		} else if (nodeName.equals("name")) {
			// pane/name können wir ignorieren
			// Log.d("ignoring 'name' tag "+ utils.getXPath(node))
			ret = null;
		} else {
			Log.e(TAG, "createPOMViewRecursive: invalid nodename:" + utils.getXPath(node) );
			Toast.makeText(this, "createPOMViewRecursive: invalid nodename:" + nodeName, Toast.LENGTH_LONG).show();
			ret = null;
		}
		return ret;
	}

	private static class MyNamespaceContext implements NamespaceContext {

		// / @prefix: das ist das was im xpath verwendet wurde (nicht das aus
		// der .xml)
		public String getNamespaceURI(String prefix) {
			if ("xi".equals(prefix)) {
				return PomAction.XincludeXMLns;
			}
			return null;
		}

		public String getPrefix(String namespaceURI) {
			return null;
		}

		public Iterator getPrefixes(String namespaceURI) {
			return null;
		}

	}

	@Override
	public void onResume() {
		super.onResume();
		Log.i(TAG, "ControlAction::onResume");
		AndroidMain.plusActivity();
	}

	@Override
	public void onPause() {
		super.onPause();
		Log.i(TAG, "ControlAction::onPause isFinishing:" + this.isFinishing());
		AndroidMain.minusActivity();
	}

	/**
	 * das rumgefummel mit onKeyDown ist weil sich das ControlAction sonst gerne
	 * auch beendet weils das onBackPressed auch bekommt ....
	 * 
	 * @see android.app.Activity#onKeyDown(int, android.view.KeyEvent)
	 */
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK)
			return true;
		return super.onKeyDown(keyCode, event);
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			this.finish();
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}

	class AvailLocosListItemAddr extends AvailLocosListItem {
		public AvailLocosListItemAddr(int addr, String name, Bitmap img,
				int speed, int funcBits) {
			super(name, img, speed, funcBits);
			this.addr = addr;
		}

		int addr;
	}

	public static class ViewHolder {
		ImageView img;
		TextView addr;
		TextView name;
		CheckBox cb;
	}

	public void onClickMultiButton(View v) {
		Intent intent = new Intent();
		ArrayList<Integer> value = new ArrayList<Integer>();
		for (int position : this.selectedLocosPos) {
			value.add(this.listAdapter.getItem(position).addr);
		}
		intent.putIntegerArrayListExtra("currAddr", value);
		intent.putExtra("currAddr", value);
		setResult(RESULT_OK, intent);
		finish();
	}

	/**
	 * Getting XML content by making HTTP Request This function will get XML by
	 * making an HTTP Request.
	 * @throws Exception wenn url nicht 200 macht
	 */
	static HashMap <String, String>xmlCache=new HashMap<String,String>();
	public static String getXmlFromUrl(String url) throws Exception {
		if(xmlCache.containsKey(url)) {
			Log.i(TAG, "fetching url (cached) ... " + url);
			return xmlCache.get(url);
		}
		/* mit apache lib
		try {
			String xml = null;
			Log.i(TAG, "fetching url ... " + surl);
			// defaultHttpClient
			DefaultHttpClient httpClient = new DefaultHttpClient();
			HttpPost httpPost = new HttpPost(url);

			HttpResponse httpResponse = httpClient.execute(httpPost);
			HttpEntity httpEntity = httpResponse.getEntity();
			xml = EntityUtils.toString(httpEntity);

			xmlCache.put(surl, xml);
			return xml;
			
		} catch (UnsupportedEncodingException e) {
			Log.e(TAG, "fetching url ... error ", e);
		} catch (ClientProtocolException e) {
			Log.e(TAG, "fetching url ... error ", e);
		} catch (IOException e) {
			Log.e(TAG, "fetching url ... error ", e);
		}
		*/
		String xml=utils.httpGet(url);
		xmlCache.put(url, xml);
		return xml;
	}

	/**
	 * Parsing XML content and getting DOM element After getting XML content we
	 * need to get the DOM element of the XML file. Below function will parse
	 * the XML content and will give you DOM element.
	 */
	public static Document getDomElement(String xml) throws Exception {
		Document doc = null;
		DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
		dbf.setIgnoringComments(true);
		// Debuglog.debugln("isXIncludeAware: "+dbf.isXIncludeAware());
		// dbf.setXIncludeAware(true);
		// Debuglog.debugln("isNamespaceAware: "+dbf.isNamespaceAware());
		dbf.setNamespaceAware(true);

		DocumentBuilder db = dbf.newDocumentBuilder();

		InputSource is = new InputSource();
		// danke windof notepad ... remove BOM:
		if (xml != null && xml.startsWith("\uFEFF")) {
			xml = xml.substring(1);
		}
		is.setCharacterStream(new StringReader(xml));
		doc = db.parse(is);

		return doc;
	}

	public void replaceIncludes(Node startNode, int depth, String which, final PomAction PA) throws Exception{
		if(startNode == null) {
			throw new Exception("replaceIncludes(Node startNode==null ...");
		}
		Log.d(TAG, "## replaceIncludes("+depth+" / " + which + ") >>>>>");
		if(depth > 4) {
			Log.e(TAG, "## replaceIncludes depth");
			return;
		}
		Element lastParent = null;
		// 2* includes auflösen ...

			// da alle include-nodes nachladen ...
			// (2012.0707: android kennt kein include:
			// includeXMLns
			String expression = "//xi:include";
			NodeList nodes;
			nodes = (NodeList) xpath.evaluate(expression, startNode, XPathConstants.NODESET);
			Log.d(TAG, "## found " + nodes.getLength() + " Elements ==== " + nodes.toString());
			for (int i = 0; i < nodes.getLength(); i++) {
				Node node = nodes.item(i);

				if (node instanceof Element) {
					Element includeChild = (Element) node;
					final String url = includeChild.getAttribute("href").replace("http://jmri.org", xmlHost);

					PA.runOnUiThread(new Runnable() {
					    public void run() {
							PA.tvInfo.setText("loading xml "+url);					    	
					    }
					});
					

					String xmlData = PomAction.getXmlFromUrl(url); // getting
														// XML
					Document includeDoc = PomAction.getDomElement(xmlData); // getting DOM element
					if(includeDoc == null) {
						throw new Exception("parsing "+url+ " failed");
					}
					replaceIncludes(includeDoc, depth+1, url, PA);
					NodeList list = includeDoc.getChildNodes();
					lastParent = (Element) includeChild.getParentNode();
					
					for(int j = 0; j < list.getLength(); j++) {
						Node newChild = list.item(j);
						if(newChild instanceof Element) {
							Log.d(TAG, "importing tag "+((Element)newChild).getTagName());
						} else {
							Log.d(TAG, "skipping import of "+newChild.getClass());
						}
						Node tempDoc = lastParent.getOwnerDocument().importNode(newChild, true);
					
					// Log.i(tag, "doc:" + doc + " startNode doc: " + startNode.getOwnerDocument() + " child doc: " + child.getOwnerDocument() +
					// 		" parent doc:"+lastParent.getOwnerDocument() +
					// 		" new doc:"+tempDoc.getOwnerDocument());
						lastParent.insertBefore(tempDoc, includeChild);
					// child.appendChild(tempDoc);
					// lastParent.appendChild(tempDoc);
					// node.appendChild(); // 0 = processing
					// info
					// Comment comment =
					// doc.createComment("Just a thought");
					// Element e =
					// doc.createElement("child");
					// e.appendChild(newChild);
					// child.appendChild(e);
					}
					lastParent.removeChild(includeChild);
				}
		}
		Log.d(TAG, "## replaceIncludes("+depth+" / " + which + ") <<<< done");
	}

}
