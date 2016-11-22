package com.topvs.client;

import android.content.Context;   
import android.database.Cursor;   
import android.database.sqlite.SQLiteDatabase;   
import android.util.Log;   
  
/**  
 * 数据库操作工具类  
 *   
 * @author daguangspecial@gmail.com  
 *   
 */  
public class DatabaseHelper 
{  	
    private static final String TAG = "DatabaseHelper";// 调试标签     
	public static final String DATABASE_NAME ="iplist.db";
    private static final String TABLE_NAME="config";
    
    SQLiteDatabase db;   
    Context context;//应用环境上下文   Activity 是其子类   
  
    DatabaseHelper(Context _context) 
    {   
        context = _context;   
        //开启数据库               
        db = context.openOrCreateDatabase(DATABASE_NAME, Context.MODE_PRIVATE,null);   
        CreateTable();   
        Log.i(TAG, "db path=" + db.getPath());   
    }   
  
    /**  
     * 建表  
     * 列名 区分大小写？  
     * 都有什么数据类型？  
     * SQLite 3   
     *  TEXT    文本  
        NUMERIC 数值  
        INTEGER 整型  
        REAL    小数  
        NONE    无类型  
     * 查询可否发送select ?  
     */  
    public void CreateTable() {   
        try { 
            db.execSQL("CREATE TABLE IF NOT EXISTS "+TABLE_NAME 
                    +"(ID INTEGER PRIMARY KEY autoincrement,"
            		+"IP VARCHAR, USER VARCHAR, PWD VARCHAR, PORT INTEGER, PROTOCOL INTEGER, PUID VARCHAR"
                    + ");"); 	//2014-04-01
//            db.execSQL("CREATE TABLE IF NOT EXISTS "+TABLE_NAME+ 
//                    " (ID INTEGER PRIMARY KEY autoincrement,"  
//                    + "IP VARCHAR, PORT INTEGER,USER VARCHAR, PWD VARCHAR"    
//                    + ");");   
            Log.i(TAG, "Create Table t_user ok");   
        } catch (Exception e) {
            Log.i(TAG, "Create Table t_user err,table exists.");   
        }   
    }   
    /**  
     * 增加数据  
     * @param id  
     * @param uname  
     * @return  
     */  
    public boolean save(String ip, String user, String pwd, int port, int protocol, String puid)
    {  
        String sql="";   
        try{ 
        	sql="insert into "+TABLE_NAME+" values(null," + "'"+ip+"','" +user+"','"+pwd+"',"
        			+port+","+protocol+",'"+puid+"')";  //2014-04-01
        	
//          sql="insert into "+TABLE_NAME+ " values(null," +
//            		"'" + ip+ "'," + port + ",'" + user + "','" + pwd +"')";   
            db.execSQL(sql);   
            Log.i(TAG,"insert Table t_user ok");   
            return true;   
               
        }catch(Exception e){   
            Log.i(TAG,"insert Table t_user err ,sql: "+sql);   
            return false;   
        }   
    }  
    
    public boolean update(int id, String ip, String user, String pwd, int port, int protocol, String puid)
    {   
        String sql="";   
        try{  
            sql = "update " + TABLE_NAME + " set IP='"+ip + "',USER='"+user + "',PWD='"+pwd + 
            		"',PORT="+port + ",PROTOCOL="+protocol + ",PUID='"+puid + "' where ID="+id;
//            sql="update "+TABLE_NAME+ " set IP='"+ip+ "', PORT=" + port +
//            		",USER = '" + user + "',PWD='" + pwd +"' where ID = "+ id;
            db.execSQL(sql);
            Log.i(TAG,"insert Table t_user ok");
            return true;
               
        }catch(Exception e){
            Log.i(TAG,"update Table  err ,sql: "+sql);
            return false;
        }   
    } 
    
    public boolean delete(int id){   
        String sql="";   
        try{   
            sql="delete from "+TABLE_NAME+ " where ID =" + id ;   
            db.execSQL(sql);   
            Log.i(TAG,"delete  ok");   
            return true;   
               
        }catch(Exception e){   
            Log.i(TAG,"delete Table  err ,sql: "+sql);   
            return false;   
        }   
    } 
    
    public boolean deleteAll(){   
        String sql="";   
        try{   
            sql="delete from "+TABLE_NAME;   
            db.execSQL(sql);   
            Log.i(TAG,"deleteAll  ok");   
            return true;   
               
        }catch(Exception e){   
            Log.i(TAG,"delete Table  err ,sql: "+sql);   
            return false;   
        }   
    } 
    /**  
     * 查询所有记录  
     *   
     * @return Cursor 指向结果记录的指针，类似于JDBC 的 ResultSet  
     */  
    public Cursor loadAll(){   
           
//      Cursor cur=db.query(TABLE_NAME, new String[]{"ID","IP","PORT","USER","PWD"}, null, null, null, null, null);   
    	Cursor cur=db.query(TABLE_NAME, new String[]{"ID", "IP", "USER", "PWD", "PORT", "PROTOCOL", "PUID"},
    			null, null, null, null, null, null); 
        
        return cur;   
    }   
    
    public Cursor queryByID(int id){   
        
        Cursor cur=db.rawQuery("select * from "+TABLE_NAME + " where ID="+id, null);   
        Log.i(TAG,"queryByID  ok");      
        return cur;
    }
    
      public void close(){   
        db.close();   
    }   
}  
