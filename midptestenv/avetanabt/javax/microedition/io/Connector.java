/**
*  (c) Copyright 2004 Avetana GmbH ALL RIGHTS RESERVED.
*
* This file is part of the Avetana bluetooth API for Linux.
*
* The Avetana bluetooth API for Linux is free software; you can redistribute it
* and/or modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* The Avetana bluetooth API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* The development of the Avetana bluetooth API is based on the work of
* Christian Lorenz (see the Javabluetooth Stack at http://www.javabluetooth.org) for some classes,
* on the work of the jbluez team (see http://jbluez.sourceforge.net/) and
* on the work of the bluez team (see the BlueZ linux Stack at http://www.bluez.org) for the C code.
* Classes, part of classes, C functions or part of C functions programmed by these teams and/or persons
* are explicitly mentioned.
*
* @author Julien Campana
*/
package javax.microedition.io;

/**
 * This class only supports the RFCOMM Protocol for the moment.
 * It will be soon extended in order to support RFCOMM and maybe OBEX.
 *
 * Remote (btspp://010203040506:1;master=false)
 * or
 * local (btspp://localhost:3B9FA89520078C303355AAA694238F07:1;name=Avetana Service;) URLs
 * are supported. The class JSR82URL verifies that the URL is a correct Bluetooth
 * connection URL, which matches the RFC 1808 specification.
 * (see http://www.w3.org/Addressing/rfc1808.txt for more information).
 *
 *
 * @todo Move auth / enc / authorize test to native code
 */
public class Connector extends de.avetana.bluetooth.connection.Connector{
 
 }

