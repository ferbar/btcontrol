package org.ferbar.utils;

import org.w3c.dom.Node;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

public class utils {

    public static String httpGet(String surl) throws Exception{
        HttpURLConnection urlConnection=null;
        try {
            URL url = new URL(surl);
            urlConnection = (HttpURLConnection) url.openConnection();
            int responseCode = urlConnection.getResponseCode();
            if (responseCode >= 400) {
                throw new Exception("error loading url "+surl);
            }
            try (
                    InputStream in = new BufferedInputStream(urlConnection.getInputStream());
                    BufferedReader reader = new BufferedReader(new InputStreamReader(in))
            ) {
                StringBuilder result = new StringBuilder();
                String line;
                while ((line = reader.readLine()) != null) {
                    result.append(line);
                }
                String xml = result.toString();
                return xml;
            }
			/*
		} catch (Exception e) {
			Log.e(TAG, "fetching url ... error ", e);
			 */
        } finally {
            if(urlConnection!=null)
                urlConnection.disconnect();
        }

    }

    public static String getXPath(Node node) {
        if (node == null) {
            return "";
        }

        StringBuilder path = new StringBuilder();
        Node current = node;

        // Schleife läuft hoch, bis das Dokument (Root) erreicht ist
        while (current != null && current.getNodeType() == Node.ELEMENT_NODE) {
            String tagName = current.getNodeName();
            int index = getElementIndex(current);

            // Baut den Pfad von hinten nach vorne auf: /element[index]
            path.insert(0, "/" + tagName + "[" + index + "]");

            current = current.getParentNode();
        }

        return path.toString();
    }

    // Hilfsmethode, um die Position (1-basiert) unter Geschwisterelementen gleichen Namens zu finden
    private static int getElementIndex(Node node) {
        int index = 1;
        String name = node.getNodeName();
        Node sibling = node.getPreviousSibling();

        while (sibling != null) {
            if (sibling.getNodeType() == Node.ELEMENT_NODE && sibling.getNodeName().equals(name)) {
                index++;
            }
            sibling = sibling.getPreviousSibling();
        }
        return index;
    }


}
