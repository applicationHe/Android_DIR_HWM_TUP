package com.topvs.client;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class BMPImage {
 
    // --- 私有常量
    private final static int BITMAP_FILE_HEADER_SIZE = 14;
    private final static int BITMAP_INFO_HEADER_SIZE = 40;


    private static int  biWidth = 720;
    private static int   biHeight = 576;

    ByteBuffer bmpBuffer = null;
    
    static byte[] data565 ;   
    
   
       
    /*
     * 因为宽高一旦确定后，便不会改变，因此运用静态方法，确定好
     * 写入固定头部信息 
     */
	public static void Init(int Width, int Height)
	{
		biWidth = Width;
		biHeight = Height;

		int biSizeImage = Width * Height * 3;
		int bfSize = BITMAP_FILE_HEADER_SIZE + BITMAP_INFO_HEADER_SIZE + Width * Height * 3 + 12;
		int bfOffBits = BITMAP_FILE_HEADER_SIZE + BITMAP_INFO_HEADER_SIZE + 12;

		ByteArrayOutputStream bas = new ByteArrayOutputStream();
		DataOutputStream dos = new DataOutputStream(bas);

		try
		{
			// writeBitmapFileHeader
			dos.write(new byte[] { 'B', 'M' });
			dos.write(intToDWord(bfSize));
			dos.write(intToWord(0));
			dos.write(intToWord(0));
			dos.write(intToDWord(bfOffBits));

			// writeBitmap_Info_Header
			dos.write(intToDWord(40));
			dos.write(intToDWord(Width));
			dos.write(intToDWord(Height));

			dos.write(intToWord(1));
			dos.write(intToWord(16));
			dos.write(intToDWord(3));

			dos.write(intToDWord(biSizeImage));
			dos.write(intToDWord(0));
			dos.write(intToDWord(0));
			dos.write(intToDWord(0));
			dos.write(intToDWord(0));

			// 写入565信息
			dos.write(intToDWord(63488));
			dos.write(intToDWord(2016));
			dos.write(intToDWord(31));

			dos.close();
			bas.close();

			data565 = bas.toByteArray();
		}
		catch (IOException e)
		{
			e.printStackTrace();
		}
	}
     

	 public BMPImage(byte[] Data){
	        bmpBuffer = ByteBuffer.allocate(BITMAP_FILE_HEADER_SIZE + BITMAP_INFO_HEADER_SIZE + 12 + biWidth*biHeight*3);
	        bmpBuffer.put(data565);
	        bmpBuffer.put(Data);//写入实际的数据	        
	    }
  
	 
	/* public byte[] getByte(){
	        return bmpBuffer.array();
	    }*/
	 

    public static byte[] getByte(byte[] Data){
        ByteBuffer bmpBuf = null;        
        bmpBuf = ByteBuffer.allocate(BITMAP_FILE_HEADER_SIZE + BITMAP_INFO_HEADER_SIZE + 12 + biWidth*biHeight*3);
        bmpBuf.put(data565);
        bmpBuf.put(Data);//写入实际的数据
        byte[] rtn =bmpBuf.array();
        bmpBuf.clear();
        return rtn;
    }

  
	private static byte[] intToWord(int parValue)
	{
		byte retValue[] = new byte[2];
		retValue[0] = (byte) (parValue & 0x00FF);
		retValue[1] = (byte) ((parValue >> 8) & 0x00FF);
		return (retValue);
	}

    private static byte[] intToDWord(int parValue) 
    {
        byte retValue[] = new byte[4];
        retValue[0] = (byte) (parValue & 0x00FF);
        retValue[1] = (byte) ((parValue >> 8) & 0x000000FF);
        retValue[2] = (byte) ((parValue >> 16) & 0x000000FF);
        retValue[3] = (byte) ((parValue >> 24) & 0x000000FF);
        return (retValue);
    }

    public void clear(){
 	   bmpBuffer.clear();
    }


}