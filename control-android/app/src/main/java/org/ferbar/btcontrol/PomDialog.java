package org.ferbar.btcontrol;

import android.app.Activity;
import android.app.Dialog;
import android.graphics.PorterDuff;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import protocol.FBTCtlMessage;
import protocol.MessageLayouts;

public class PomDialog {
    final static String TAG="btcontrol.PomDialog";
    public static void show(Activity activity, String cv) {
        ControlAction.AvailLocosListItem lok=ControlAction.availLocos.get(ControlAction.currSelectedAddr.get(0));
        if(lok != null) {
            final Dialog dialog = new Dialog(activity);

            dialog.setContentView(R.layout.pom);
            dialog.setTitle("Programming on the Main");

            // nettes bild + lokname setzen
            TextView tv = (TextView) dialog.findViewById(R.id.textView1);
            tv.setText(lok.name);
            ImageView iv = (ImageView) dialog.findViewById(R.id.imageViewLok);
            iv.setImageBitmap(lok.img);

            EditText tv_cv=(EditText) dialog.findViewById(R.id.editTextCV);
            tv_cv.setText(cv);
            if(!cv.isEmpty()) {
                try {
                    FBTCtlMessage msg = new FBTCtlMessage();
                    msg.setType(MessageLayouts.messageTypeID("POM"));
                    msg.get("addr").set(ControlAction.currSelectedAddr.get(0));
                    msg.get("cv").set(Integer.parseInt(cv));
                    msg.get("value").set(-1); //query only
                    AndroidMain.btcomm.addCmdToQueue(msg, new BTcommThread.BtCommCallback() {
                        public void BTCallback(FBTCtlMessage reply) {
                            Log.d(TAG, "got cv reply");
                            activity.runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    int cv= 0;
                                    try {
                                        cv = reply.get("value").getIntVal();
                                        EditText tv_value=(EditText) dialog.findViewById(R.id.editTextValue);
                                        tv_value.setText(""+cv);
                                    } catch (Exception e) {
                                        Log.e(TAG,"error getting cv value", e);
                                    }
                                }
                            });
                        }
                    });
                } catch (Exception e) {
                    Log.e(TAG,"error query cv",e);
                }
            }


            // onChange -> valueBits updaten
            ((EditText)dialog.findViewById(R.id.editTextValue)).addTextChangedListener(new TextWatcher() {
                public void afterTextChanged(Editable s) {
                    View v=(View)(EditText) dialog.findViewById(R.id.editTextValue);
                    String value=s.toString();
                    int vi=0;
                    try{
                        vi=Integer.parseInt(value);
                        if(vi > 255) {
                            v.getBackground().setColorFilter(0xFFFF0000, PorterDuff.Mode.MULTIPLY);
                        } else {
                            v.getBackground().setColorFilter(null);
                            if(v.hasFocus()) {
                                String valueBits="";
                                for(int i=0; i < 8; i++) {
                                    valueBits=(((vi >> i) & 1) == 1 ? 1 : 0 ) + valueBits;
                                }
                                EditText editTextValueBits=(EditText) dialog.findViewById(R.id.editTextValueBits);
                                editTextValueBits.setText(valueBits);
                            }
                        }
                    } catch(Exception e) {
                        v.getBackground().setColorFilter(0xFFFF0000, PorterDuff.Mode.MULTIPLY);
                    }
                }
                public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
                public void onTextChanged(CharSequence s, int start, int before, int count) { }
            });
            // onChange -> value updaten
            ((EditText)dialog.findViewById(R.id.editTextValueBits)).addTextChangedListener(new TextWatcher() {
                public void afterTextChanged(Editable s) {
                    View v=(View)(EditText) dialog.findViewById(R.id.editTextValueBits);
                    if(v.hasFocus()) {
                        String valueBits=s.toString();
                        int vbi;
                        try {
                            vbi=Integer.parseInt(valueBits,2);
                            v.getBackground().setColorFilter(null);
                        } catch(Exception e) {
                            vbi=0;
                            v.getBackground().setColorFilter(0xFFFF0000, PorterDuff.Mode.MULTIPLY);
                        }
                        String value=""+vbi;
                        EditText editTextValueBits=(EditText) dialog.findViewById(R.id.editTextValue);
                        editTextValueBits.setText(value);
                    }
                }
                public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
                public void onTextChanged(CharSequence s, int start, int before, int count) { }
            });
            ((Button)dialog.findViewById(R.id.buttonGo)).setOnClickListener(new View.OnClickListener() {
                public void onClick(View goButton) {
                    try {
                        EditText v=(EditText) dialog.findViewById(R.id.editTextCV);
                        String sCV=v.getText().toString();
                        if(sCV.length()==0) {
                            Toast.makeText(dialog.getContext(), "CV number empty", Toast.LENGTH_LONG).show();
                            return;
                        }
                        int cv=Integer.parseInt(sCV);

                        v=(EditText) dialog.findViewById(R.id.editTextValue);
                        String sValue=v.getText().toString();
                        int value;
                        if(sValue.length() == 0) {
                            value=-1;
                        } else {
                            value=Integer.parseInt(sValue);
                        }

                        FBTCtlMessage msg = new FBTCtlMessage();
                        msg.setType(MessageLayouts.messageTypeID("POM"));
                        msg.get("addr").set(ControlAction.currSelectedAddr.get(0));
                        msg.get("cv").set(cv);
                        msg.get("value").set(value);

                        FBTCtlMessage reply = AndroidMain.btcomm.execCmd(msg);
                        if(reply != null) {
                            Toast.makeText(dialog.getContext(), "CV "+cv+" = "+value+" gesendet", Toast.LENGTH_LONG).show();
                            v.setText(""+reply.get("value").getIntVal());
                        }
                    } catch (Exception e) {
                        // TODO Auto-generated catch block
                        Log.e(TAG, "POM Exception", e);
                        Toast.makeText(dialog.getContext(), "POM Exception "+e.getMessage(), Toast.LENGTH_SHORT).show();
                    }

                }
            });
            dialog.setOwnerActivity(activity);
            dialog.show();
        } else {
            Toast.makeText(activity, "keine lok", Toast.LENGTH_LONG).show();
        }
    }
}
