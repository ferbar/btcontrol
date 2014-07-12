/**
 * Programming on the Main - Interface
 * ladet die decoder definition von jmri.org runter
 */

package com.example.helloandroid;

import java.io.IOException;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;
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
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.util.EntityUtils;
import org.w3c.dom.Comment;
import org.w3c.dom.DOMException;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

import com.example.helloandroid.ControllAction.AvailLocosListItem;

import btcontrol.Debuglog;
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


@TargetApi(8)
public class PomAction extends Activity {

	ArrayAdapter<AvailLocosListItemAddr> listAdapter=null;
	Object listAdapter_notify=new Object();
	private LayoutInflater mInflater;
	
	List<Integer> selectedLocosPos;
	
    static final String URL = "http://jmri.org/xml/decoders/Zimo_Unified_software_MX690Sv30.xml";
	static final String XincludeXMLns = "http://www.w3.org/2001/XInclude";
    
    TabHost mTabHost;
    static Document doc=null;
// TODO:    static Document docPane=null;
    
    static HashMap<String, View> tabViews = new HashMap<String, View>();
    static ArrayList<String> tabTabs = new ArrayList<String>();
    
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.pom_neu);
        
        this.mTabHost = (TabHost) findViewById(android.R.id.tabhost);
        this.mTabHost.setup();
        
        // JMRI decoder beschreibung laden: 
        if(doc == null) {
	        // XMLParser parser = new XMLParser();
	        String xml = this.getXmlFromUrl(PomAction.URL); // getting XML
	        this.doc = this.getDomElement(xml); // getting DOM element
	         
	        // NodeList nl = doc.getElementsByTagName(KEY_ITEM);
	        
	        Log.d("xml",doc.toString());
	        
	        XPath xpath = XPathFactory.newInstance().newXPath();
	        xpath.setNamespaceContext(new MyNamespaceContext());
			try {
	            // da alle include-nodes nachladen ... (2012.0707: android kennt kein include: 
		        // includeXMLns
		        String expression = "//xi:include";
				NodeList nodes = (NodeList) xpath.evaluate(expression, doc, XPathConstants.NODESET);
				Log.d("test", nodes.toString());
				Element lastParent=null;
		        for(int i = 0; i < nodes.getLength(); i++) {
		            Node node = nodes.item(i);
	
		            if (node instanceof Element) {
		                Element child = (Element)node;
		                String url = child.getAttribute("href");
		                xml = this.getXmlFromUrl(url); // getting XML
		                Document subdoc = this.getDomElement(xml); // getting DOM element
		                NodeList list = subdoc.getChildNodes();
		                Node newChild = list.item(1);
		                Node tempDoc = doc.importNode(newChild, true);
		                lastParent=(Element) child.getParentNode();
		                // doc.replaceChild(tempDoc, child);
		                // child.appendChild(tempDoc);
		                lastParent.appendChild(tempDoc);
		                // node.appendChild(); // 0 = processing info
		                // Comment comment = doc.createComment("Just a thought");
		                // Element e = doc.createElement("child");
		                // e.appendChild(newChild);
		                // child.appendChild(e);
		            }
		        }
		        
		        // includes aus Comprehensive.xml, an letztes child anhängen
		        String extra[] = { "http://jmri.org/xml/programmers/parts/BasicPane.xml",
		        	"http://jmri.org/xml/programmers/parts/MotorPane.xml",
		        	"http://jmri.org/xml/programmers/parts/BasicSpeedControlPane.xml",
		       		"http://jmri.org/xml/programmers/parts/SpeedTablePane.xml",
		       		"http://jmri.org/xml/programmers/parts/FunctionMapPane.xml",
		       		"http://jmri.org/xml/programmers/parts/LightsPane.xml",
		       		"http://jmri.org/xml/programmers/parts/AnalogControlsPane.xml",
		       		"http://jmri.org/xml/programmers/parts/ConsistPane.xml",
		       		"http://jmri.org/xml/programmers/parts/AdvancedPane.xml",
		       		"http://jmri.org/xml/programmers/parts/SoundPane.xml",
		       		"http://jmri.org/xml/programmers/parts/SoundLevelsPane.xml",
		       		"http://jmri.org/xml/programmers/parts/CVsPane.xml"};
		        for(String url : extra) {
	            	xml = this.getXmlFromUrl(url); // getting XML
	                Document subdoc = this.getDomElement(xml); // getting DOM element
	                NodeList list = subdoc.getChildNodes();
	                Node newChild = null;
	                if(list.item(0) instanceof Element)
	                	 newChild = list.item(0);
	                else if (list.item(1) instanceof Element)
	                	 newChild = list.item(1);
	                if(newChild != null) {
	                	Node tempDoc = doc.importNode(newChild, true);
	                	lastParent.appendChild(tempDoc);
	                }
		        }
	
		        
			} catch (XPathExpressionException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (DOMException e) {
				e.printStackTrace();
				Debuglog.debugln("DOMException " + e.toString());
				// TODO Auto-generated catch block
			}
	        
			try {
				// panes raussuchen und dann die tabs anlegen:
		        String expression = "//pane";
				NodeList nodes;
				nodes = (NodeList) xpath.evaluate(expression, doc, XPathConstants.NODESET);
				Log.d("test", nodes.toString());
		        //NodeList nodes = doc.getElementsByTagName("pane");
		        for(int i = 0; i < nodes.getLength(); i++) {
		            Node node = nodes.item(i);
	
		            if (node instanceof Element) {
		            	Element child = (Element)node;
		            	String name = child.getAttribute("name");
		            	PomAction.tabTabs.add(name);
		            	
		            }
		        }

			} catch (XPathExpressionException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
        } else { // nur tabs anlegen:
        	for(String name :PomAction.tabTabs) {
        		this.setupTab(name);
        	}
        }
        
        this.selectedLocosPos = new ArrayList<Integer>();
        
    	// mInflater = (LayoutInflater)getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        
        // registerForContextMenu(getListView());
        /*
        Bundle bundle = getIntent().getExtras();
        if(bundle != null) {
        	this.selectMehrfachsteuerung=bundle.getBoolean("Mehrfachsteuerung",false);
        	ImageButton ib = (ImageButton) this.findViewById(R.id.imageButtonStartMulti);
        	this.orgStartMultiButtonImage = ib.getDrawable();
        } else {
        	this.findViewById(R.id.linearLayoutMulti).setVisibility(View.GONE);
        }
        */
        
        // startFillData();
        // String[] mStrings = new String[]{"Android", "Google", "Eclipse"};
		// this.setListAdapter(new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, mStrings));
    }
    
	/**
	 * legt ein neues tab an, tabhost muss inited sein + setup() aufgerufen
	 * 
	 * @param view
	 *            -> content
	 * @param tag
	 *            -> tab name
	 */
	private void setupTab(final String tag) {

		TabSpec tab = this.mTabHost.newTabSpec(tag);
		// View tabview = createTabView(this.mTabHost.getContext(), tag);
		// newlines einbauen:
		Pattern p = Pattern.compile("^(\\w{3,})([\\s/-]+)(\\w{3,})(([\\s/-]+)(.*))?$");
		Matcher m = p.matcher(tag);
		String tabIndicator = tag;
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
				View tabView = PomAction.this.createPOMView(tag);
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
	 * @param context
	 * @param text
	 * @return
	 *
	private static View createTabView(final Context context, String text) {

		LinearLayout view = new LinearLayout(context); 
/ *			
			LayoutInflater.from(context)
				.inflate(R.layout.tabs_bg, null);

		TextView tv = (TextView) view.findViewById(R.id.tabsText);
* /
		TextView tv = new TextView(context);
		// ~ alle 4 Zeichen ein newline reinhaun:
		text=text.replaceAll("^(\\w{3,})([\\s\\/]+)(.*)$","$1\n$3");

		tv.setText(text);
		view.addView(tv);

		return view;

	}
	*/

	/**
	 * legt das view dings für die Tab Seite an
	 * @param tag <pane name="_tag_">
	 * @return View
	 */
	private View createPOMView(String tag) {
		LinearLayout ret = new LinearLayout(this);
		ret.setOrientation(LinearLayout.VERTICAL);
    	TextView tv = new TextView(this);
    	tv.setText(tag);
    	ret.addView(tv);
    	
    	XPath xpath = XPathFactory.newInstance().newXPath();
    	String expression="//pane[@name='"+tag+"']"; // /decoder-config/pane müsste auch gehn
		Element node;
		try {
			node = (Element) xpath.evaluate(expression, PomAction.doc, XPathConstants.NODE);
			if(node == null) {
				Debuglog.debugln("createPOMView error finding pane:"+tag+"");
			} else {
				Log.d("test", node.toString());
				ScrollView v = new ScrollView(this);
				v.setFillViewport(true);
				HorizontalScrollView hv = new HorizontalScrollView(this);
				hv.setFillViewport(true);
				LayoutParams params=new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
				v.setLayoutParams(params);
				hv.setLayoutParams(params);
				hv.addView(createPOMViewRecursive(node));
			    v.addView(hv);
				ret.addView(v);
			}
				
		} catch (XPathExpressionException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    	return ret;
	}

	private View createPOMViewRecursive(Element node) throws XPathExpressionException {
		View ret ;
		String nodeName=node.getNodeName();
		if(nodeName.equals("column") || nodeName.equals("row") || nodeName.equals("pane")) {
			LinearLayout ll_ret=new LinearLayout(this);
			if(nodeName.equals("column")) {
				// TODO: vertical setzen
				ll_ret.setOrientation(LinearLayout.VERTICAL);
			}
			for(Node childNode=(Node) node.getFirstChild(); childNode!=null; childNode=(Node)childNode.getNextSibling()){
				// Do something with childNode...
				if(childNode instanceof Element) {
					Element e=(Element) childNode;
					View v=createPOMViewRecursive(e);
					if(v != null) {
						ll_ret.addView(v);
					}
				}
			}
			ret=ll_ret;
		} else if(nodeName.equals("label")) {
			TextView tv=new TextView(this);
			tv.setText(node.getAttribute("label"));
			ret=tv;
		} else if(nodeName.equals("separator")) {
			View v=new View(this);
			LayoutParams params;
			if(node.getParentNode().getNodeName().equals("row")) {
				params=new LayoutParams(2, LayoutParams.MATCH_PARENT);
			} else {
				params=new LayoutParams(LayoutParams.MATCH_PARENT, 2);
			}
			v.setLayoutParams(params);
			int bgcolor=0xFF909090;
			v.setBackgroundColor(bgcolor);
			ret=v;
		} else if(nodeName.equals("display")) {
			String item=node.getAttribute("item");
			XPath xpath = XPathFactory.newInstance().newXPath();
	    	// String expression="//variable[@label='"+item+"']";
	    	String expression="/decoder-config/variables/variable[@item='"+item+"']";
// 	--- da muss man rausfinden ob a) voller xpath funzt, b) @item= oder @label=
			// variable label="Acceleration" CV="3" default="12" item="Accel"
			Element variableNode = (Element) xpath.evaluate(expression, this.doc, XPathConstants.NODE);
			if(variableNode != null) {
				LinearLayout ll_ret=new LinearLayout(this);
				TextView label=new TextView(this);
				String format=node.getAttribute("format");
				label.setText(variableNode.getAttribute("label") + " (#"+variableNode.getAttribute("CV")+")");
				ll_ret.addView(label);
				TextView comment=new TextView(this);
				ll_ret.addView(comment);
				ret=ll_ret;
			} else {
				Debuglog.debugln("error finding " + expression );
				ret=null;
			}
		} else {
			Debuglog.debugln("createPOMViewRecursive: invalid nodename:"+nodeName);
			// TODO: error handling
			ret= null;
		}
		return ret;
	}
	
    private static class MyNamespaceContext implements NamespaceContext {

    	/// @prefix: das ist das was im xpath verwendet wurde (nicht das aus der .xml)
        public String getNamespaceURI(String prefix) {
            if("xi".equals(prefix)) {
                return PomAction.XincludeXMLns ;
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
		System.out.println("ControllAction::onResume");
		AndroidMain.plusActivity();
	}
	@Override
	public void onPause() {
		super.onPause();
		System.out.println("ControllAction::onPause isFinishing:"+this.isFinishing());
		AndroidMain.minusActivity();
	}

	/**
	 * das rumgefummel mit onKeyDown ist weil sich das ControllAction sonst gerne auch beendet weils das onBackPressed auch bekommt ....
	 * @see android.app.Activity#onKeyDown(int, android.view.KeyEvent)
	 */
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if(keyCode == KeyEvent.KEYCODE_BACK)
			return true; 
		return super.onKeyDown(keyCode, event);
	}
	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if(keyCode == KeyEvent.KEYCODE_BACK) {
			this.finish();
			return true; 
		}
		return super.onKeyDown(keyCode, event);
	}
	
    class AvailLocosListItemAddr extends AvailLocosListItem {
    	public AvailLocosListItemAddr(int addr, String name, Bitmap img, int speed, int funcBits) {
    		super(name, img, speed, funcBits);
    		this.addr=addr;
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
        Intent intent=new Intent();
        ArrayList<Integer> value = new ArrayList<Integer>();
        for(int position : this.selectedLocosPos) {
        	value.add(this.listAdapter.getItem(position).addr);
        }
        intent.putIntegerArrayListExtra("currAddr", value);
        intent.putExtra("currAddr", value);
        setResult(RESULT_OK, intent);
        finish();    	
    }
    
    /** 
     * Getting XML content by making HTTP Request
This function will get XML by making an HTTP Request.
     */
    public String getXmlFromUrl(String url) {
        String xml = null;
 
        try {
            Debuglog.debugln("fetching url ... " + url);
            // defaultHttpClient
            DefaultHttpClient httpClient = new DefaultHttpClient();
            HttpPost httpPost = new HttpPost(url);
 
            HttpResponse httpResponse = httpClient.execute(httpPost);
            HttpEntity httpEntity = httpResponse.getEntity();
            xml = EntityUtils.toString(httpEntity);
 
        } catch (UnsupportedEncodingException e) {
            Debuglog.debugln("fetching url ... error ");
            e.printStackTrace();
        } catch (ClientProtocolException e) {
            Debuglog.debugln("fetching url ... error ");
            e.printStackTrace();
        } catch (IOException e) {
            Debuglog.debugln("fetching url ... error ");
            e.printStackTrace();
        }
        // return XML
        return xml;
    }
    
    /**
     * Parsing XML content and getting DOM element
After getting XML content we need to get the DOM element of the XML file. Below function will parse the XML content and will give you DOM element.
     */
    public Document getDomElement(String xml){
        Document doc = null;
        DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
        dbf.setIgnoringComments(true);
        // Debuglog.debugln("isXIncludeAware: "+dbf.isXIncludeAware()); dbf.setXIncludeAware(true);
        // Debuglog.debugln("isNamespaceAware: "+dbf.isNamespaceAware());
        dbf.setNamespaceAware(true);
        try {
 
            DocumentBuilder db = dbf.newDocumentBuilder();
 
            InputSource is = new InputSource();
                is.setCharacterStream(new StringReader(xml));
                doc = db.parse(is); 
 
            } catch (ParserConfigurationException e) {
                Log.e("Error: ", e.getMessage());
                return null;
            } catch (SAXException e) {
                Log.e("Error: ", e.getMessage());
                return null;
            } catch (IOException e) {
                Log.e("Error: ", e.getMessage());
                return null;
            }
                // return DOM
            return doc;
    }
    
}
