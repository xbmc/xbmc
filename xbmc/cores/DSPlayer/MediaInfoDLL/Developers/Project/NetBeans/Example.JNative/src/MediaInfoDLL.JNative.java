/**
 * MediaInfoDLL - All info about media files, for DLL (JNA version)
 *
 * Copyright (C) 2006-2006 Bro3, bro3@users.sourceforge.net
 * Copyright (C) 2006-2009 Jerome Martinez, Zen@MediaArea.net
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser Public License as published by the Free Software
 * Foundation; either version 2.1, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Boston, MA 02111.
 *
 **/

import org.xvolks.jnative.JNative;
import org.xvolks.jnative.Type;
import org.xvolks.jnative.pointers.Pointer;
import org.xvolks.jnative.pointers.memory.MemoryBlockFactory;
import org.xvolks.jnative.exceptions.NativeException;
import org.xvolks.jnative.pointers.memory.NativeMemoryBlock;

/**
 * Class to retrieve info about media files.
 * MediaInfo library (http://mediainfo.sourceforge.net) is used
 * by the help of JNative (http://jnative.sourceforge.net)
 * to obtain technical the info about the files.
 *
 * @author bro3@users.sourceforge.net
 * @author zen@mediaarea.net
 */
class MediaInfo
{
    
    /* static_fields */
    
    final public static int Stream_General       = 0;
    final public static int Stream_Video         = 1;
    final public static int Stream_Audio         = 2;
    final public static int Stream_Text          = 3;
    final public static int Stream_Chapters      = 4;
    final public static int Stream_Image         = 5;
    final public static int Stream_Menu          = 6;
    final public static int Stream_Max           = 7;

    final public static int Info_Name            = 0;
    final public static int Info_Text            = 1;
    final public static int Info_Measure         = 2;
    final public static int Info_Options         = 3;
    final public static int Info_Name_Text       = 4;
    final public static int Info_Measure_Text    = 5;
    final public static int Info_Info            = 6;
    final public static int Info_HowTo           = 7;
    final public static int Info_Max             = 8;
    
    
    /* The MediaInfo handle */
    private String handle = null;
    private JNative new_jnative;

    /* The library to be used */
    private static String libraryName = "";

    
    /**
     * Constructor that initializes the new MediaInfo object.
     * @throws NativeException  JNative Exception.
     */
    public MediaInfo() throws NativeException, Exception
    {
        setLibraryName();
        New();
    }


    /**
     * Constructor that initializes the new MediaInfo object.
     * @param libraryName       name of libarary to be used
     * @throws NativeException  JNative Exception
     */
    public MediaInfo(String libraryName) throws NativeException, Exception
    {
        setLibraryName(libraryName);
        New();
    }


    /**
     * Method New initializes the MediaInfo handle
     * @throws NativeException  JNative Exception
     */
    private void New() throws NativeException, Exception
    {
        /* Getting the handle */
        new_jnative = new JNative(libraryName, "MediaInfoA_New");
        new_jnative.setRetVal(Type.INT);
        new_jnative.invoke();
        handle = new_jnative.getRetVal();
        Option("CharSet", "UTF-8");
    }
    
    
    /**
     * Opens a media file.
     * Overloads method {@link #Open(int, int, int, int)}
     * @param  begin                          buffer with the begining of datas
     * @param  beginSize                      size of begin
     * @return                                1 for success and 0 for failure
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
     * @see                                   #Open(int, int, int, int)
     */
    public int Open(int begin, int beginSize) throws HandleNotInitializedException, NativeException, Exception
    {
        return Open(begin, beginSize, 0, 0);
    }
    

    /**
     * Opens a media file.
     * @param  begin                          buffer with the begining of datas
     * @param  beginSize                      size of begin
     * @param  end                            buffer with the end of datas
     * @param  endSize                        size of end
     * @return                                1 for success and 0 for failure
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
     */
    public int Open(int begin, int beginSize, int end, int endSize) throws HandleNotInitializedException, NativeException, Exception
    {
        if (handle == null)
            throw new HandleNotInitializedException("Handle is not initialized.");

        /*JNative call */
        JNative jnative = new JNative(libraryName, "MediaInfoA_Open_Buffer");
        jnative.setRetVal(Type.INT);
        jnative.setParameter(0, Type.INT, handle);
        jnative.setParameter(1, Type.INT, String.valueOf(begin));
        jnative.setParameter(2, Type.INT, String.valueOf(beginSize));
        jnative.setParameter(3, Type.INT, String.valueOf(end));
        jnative.setParameter(4, Type.INT, String.valueOf(endSize));
        jnative.invoke();
        
        /* Retrieving data */
        int ret = Integer.parseInt(jnative.getRetVal());

        return ret;
    }


    /**
     * Opens a media file.
     * @param  filename                       the filename
     * @return                                1 for success and 0 for failure
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
     */
    public int Open(String filename) throws HandleNotInitializedException, NativeException, Exception
    {
        if (handle == null)
            throw new HandleNotInitializedException("Handle is not initialized.");

        /* Setting the memory with the byte array returned in UTF-8 format */
        Pointer fileNamePointer = createPointer(filename);

        /*JNative call */
        JNative jnative = new JNative(libraryName, "MediaInfoA_Open");
        jnative.setRetVal(Type.INT);
        jnative.setParameter(0, Type.INT, handle);
        jnative.setParameter(1, fileNamePointer);
        jnative.invoke();

        /* Retrieving data */
        int ret = Integer.parseInt(jnative.getRetVal());

        return ret;
    }


    /**
     * Gets the file info, (if available) according to the previous options set by {@link #Option(String, String)}
     * @return                                the file info
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
     */
    public String Inform() throws HandleNotInitializedException, NativeException, Exception
    {
        if (handle == null)
            throw new HandleNotInitializedException("Handle is not initialized.");

        /*JNative call */
        JNative jnative = new JNative(libraryName, "MediaInfoA_Inform");
        jnative.setRetVal(Type.INT);
        jnative.setParameter(0, Type.INT, handle);
        jnative.setParameter(1, Type.INT, "0"); //Necessary for backward compatibility
        jnative.invoke();

        /* Retrieving data */
        String ret = retrieveString(jnative);

        return ret;
    }


    /**
     * Gets the specific info according to the parameters.
     * Overloads method {@link #Get(int, int, String, int, int)}.
     * @param streamKind                      type of stream. Can be any of the Stream_XX values {@link <a href="#field_detail">Field details</a>}
     * @param streamNumber                    stream number to process
     * @param parameter                       parameter string (list of strings is available with Option("Info_Parameters");
     * @return                                information
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
     * @see                                   #Get(int, int, String, int, int)
     */
    public String Get(int streamKind, int streamNumber, String parameter) throws HandleNotInitializedException, NativeException, Exception
    {
        return Get(streamKind, streamNumber, parameter, MediaInfo.Info_Name, MediaInfo.Info_Text);
    }


    /**
     * Gets the specific info according to the parameters.
     * Overloads method {@link #Get(int, int, String, int, int)}
     * @param streamKind                      type of stream. Can be any of the Stream_XX values {@link <a href="#field_detail">Field details</a>}
     * @param streamNumber                    stream to process
     * @param parameter                       parameter string (list of strings is available with Option("Info_Parameters");
     * @param infoKind                        type of info. Can be any of the Info_XX values {@link <a href="#field_detail">Field details</a>}
     * @return                                desired information
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
     * @see                                   #Get(int, int, String, int, int)
     */
    public String Get(int streamKind, int streamNumber, String parameter, int infoKind) throws HandleNotInitializedException, NativeException, Exception
    {
        return Get(streamKind, streamNumber, parameter, infoKind, MediaInfo.Info_Name);
    }
    
    
    /**
     * Gets the specific file info according to the parameters.
     * @param streamKind                      type of stream. Can be any of the Stream_XX values {@link <a href="#field_detail">Field details</a>}
     * @param streamNumber                    stream to process
     * @param parameter                       parameter string (list of strings is available with Option("Info_Parameters");
     * @param infoKind                        type of info. Can be any of the Info_XX values {@link <a href="#field_detail">Field details</a>}
     * @param searchKind                      type of search. Can be any of the Info_XX values {@link <a href="#field_detail">Field details</a>}
     * @return                                desired information
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
     */
    public String Get(int streamKind, int streamNumber, String parameter, int infoKind, int searchKind) throws HandleNotInitializedException, NativeException, Exception
    {
        if (handle == null)
            throw new HandleNotInitializedException("Handle is not initialized.");

        /* Setting the memory with the byte array returned in UTF-8 format */
        Pointer parameterPointer = createPointer(parameter);

        /*JNative call */
        JNative jnative = new JNative(libraryName, "MediaInfoA_Get");
        jnative.setRetVal(Type.INT);
        jnative.setParameter(0, Type.INT, handle);
        jnative.setParameter(1, Type.INT, String.valueOf(streamKind));
        jnative.setParameter(2, Type.INT, String.valueOf(streamNumber));
        jnative.setParameter(3, parameterPointer);
        jnative.setParameter(4, Type.INT, String.valueOf(infoKind));
        jnative.setParameter(5, Type.INT, String.valueOf(searchKind));
        jnative.invoke();

        /* Retrieving data */
        String ret = retrieveString(jnative);

        return ret;
    }
    
    
    /**
     * Gets the specific file info according to the parameters.
     * Overloads method {@link #Get(int, int, int, int)}.
     * @param streamKind                      type of stream. Can be any of the Stream_XX values {@link <a href="#field_detail">Field details</a>}
     * @param streamNumber                    stream to process
     * @param parameter                       parameter position (count of parameters is available with Count_Get(streamKind, streamNumber) )
     * @return                                desired information
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
     * @see                                   #Get(int, int, int, int)
    */
    public String Get(int streamKind, int streamNumber, int parameter) throws HandleNotInitializedException, NativeException, Exception
    {
        return Get(streamKind, streamNumber, parameter, MediaInfo.Info_Text);
    }
    
    
    /**
     * Gets the specific file info according to the parameters.
     * @param streamKind                      type of stream. Can be any of the Stream_XX values {@link <a href="#field_detail">Field details</a>}
     * @param streamNumber                    stream to process
     * @param parameter                       parameter position (count of parameters is available with Count_Get(streamKind, streamNumber) )
     * @param infoKind                        type of info. Can be any of the Info_XX values {@link <a href="#field_detail">Field details</a>}
     * @return                                desired information
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
    */
    public String Get(int streamKind, int streamNumber, int parameter, int infoKind) throws HandleNotInitializedException, NativeException, Exception
    {
        if (handle == null)
            throw new HandleNotInitializedException("Handle is not initialized.");
     
        /*JNative call */
        JNative jnative = new JNative(libraryName, "MediaInfoA_GetI");
        jnative.setRetVal(Type.INT);
        jnative.setParameter(0, Type.INT, handle);
        jnative.setParameter(1, Type.INT, String.valueOf(streamKind));
        jnative.setParameter(2, Type.INT, String.valueOf(streamNumber));
        jnative.setParameter(3, Type.INT, String.valueOf(parameter));
        jnative.setParameter(4, Type.INT, String.valueOf(infoKind));
        jnative.invoke();

        /* Retrieving data */
        String ret = retrieveString(jnative);

        return ret;
    }
    
    
    /**
     * Sets the option
     * Overloads method {@link #Option(String, String)}
     * @param option                          name of option
     * @return                                desired information or status of the option
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
     * @see #Option(String, String)
     */
    public String Option(String option) throws HandleNotInitializedException, NativeException, Exception
    {
        return Option(option, "");
    }
    
    
    /**
     * Sets the option with value
     * @param option                          name of option
     * @param value                           option value
     * @return                                desired information or status of the option
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
    */
    public String Option(String option, String value) throws HandleNotInitializedException, NativeException, Exception
    {
        if (handle == null)
            throw new HandleNotInitializedException("Handle is not initialized.");

        /* Setting the memory with the byte array returned in UTF-8 format */
        Pointer optionPointer = createPointer(option);
        Pointer valuePointer = createPointer(value);

        /*JNative call */
        JNative jnative = new JNative(libraryName, "MediaInfoA_Option");
        jnative.setRetVal(Type.INT);
        jnative.setParameter(0, Type.INT, handle);
        jnative.setParameter(1, optionPointer);
        jnative.setParameter(2, valuePointer);
        jnative.invoke();

        /* Retrieving data */
        String ret = retrieveString(jnative);

        return ret;
    }
    

    /**
     * Sets the option (you do not need to create a MediaInfo handle)
     * Overloads method {@link #Option_Static(String, String)}
     * @param option                          name of option
     * @return                                desired information or status of the option
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
     * @see                                   #Option_Static(String, String)
     */
    static public String Option_Static(String option) throws HandleNotInitializedException, NativeException, Exception
    {
        return Option_Static(option, "");
    }
    
    
    /**
     * Sets the option (you do not need to create a MediaInfo handle)
     * @param option                          name of option
     * @param value                           option value
     * @return                                desired information or status of the option
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
    */
    static public String Option_Static(String option, String value) throws HandleNotInitializedException, NativeException, Exception
    {
        if (libraryName.equals(""))
            setLibraryName();

        /* Setting the memory with the byte array returned in UTF-8 format */
        Pointer optionPointer = createPointer(option);
        Pointer valuePointer = createPointer(value);

        /*JNative call */
        JNative jnative = new JNative(libraryName, "MediaInfoA_Option");
        jnative.setRetVal(Type.INT);
        jnative.setParameter(0, Type.INT, "0");
        jnative.setParameter(1, optionPointer);
        jnative.setParameter(2, valuePointer);
        jnative.invoke();

        /* Retrieving data */
        String ret = retrieveString(jnative);

        return ret;
    }


    /**
     * Gets the state of the libaray
     * @return                                state of the library (between 0 and 10000)
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
    */
    public int State_Get() throws HandleNotInitializedException, NativeException, Exception
    {
        if (handle == null)
            throw new HandleNotInitializedException("Handle is not initialized.");

        /*JNative call */
        JNative jnative = new JNative(libraryName, "MediaInfoA_State_Get");
        jnative.setRetVal(Type.INT);
        jnative.setParameter(0, Type.INT, handle);
        jnative.invoke();

        /* Retrieving data */
        int ret = Integer.parseInt(jnative.getRetVal());

        return ret;
    }
    
    
    /**
     * Gets the count of streams
     * Overloads method {@link #Count_Get(int, int)}.
     * @param streamKind                      type of stream. Can be any of the Stream_XX values {@link <a href="#field_detail">Field details</a>}
     * @return                                count of streams
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
     * @see                                   #Count_Get(int, int)
     */
    public int Count_Get(int streamKind) throws HandleNotInitializedException, NativeException, Exception
    {
        return Count_Get(streamKind, -1);
    }
    
    
    /**
     * Gets the count of streams
     * @param streamKind                      type of stream. Can be any of the Stream_XX values {@link <a href="#field_detail">Field details</a>}
     * @param streamNumber                    stream to process
     * @return                                count of parameters for a specific stream
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
     */
    public int Count_Get(int streamKind, int streamNumber) throws HandleNotInitializedException, NativeException, Exception
    {
        if (handle == null)
            throw new HandleNotInitializedException("Handle is not initialized.");

        /*JNative call */
        JNative jnative = new JNative(libraryName, "MediaInfoA_Count_Get");
        jnative.setRetVal(Type.INT);
        jnative.setParameter(0, Type.INT, handle);
        jnative.setParameter(1, Type.INT, String.valueOf(streamKind));
        jnative.setParameter(2, Type.INT, String.valueOf(streamNumber));
        jnative.invoke();

        /* Retrieving data */
        int retval = Integer.parseInt(jnative.getRetVal());

        return retval;
    }


    /**
     * Deletes the handle
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
     */
    protected void finalize() throws HandleNotInitializedException, NativeException, Exception 
    {
        if (handle == null)
            throw new HandleNotInitializedException("Handle is not initialized.");

        /*JNative call */
        JNative jnative = new JNative(libraryName, "MediaInfoA_Delete");
        jnative.setParameter(0, Type.INT, handle);
        jnative.invoke();
    }


    /**
     * Closes the handle
     * @throws HandleNotInitializedException  if the handle is null
     * @throws NativeException                JNative Exception
     */
    public void Close() throws HandleNotInitializedException, NativeException, Exception
    {
        if (handle == null)
            throw new HandleNotInitializedException("Handle is not initialized.");

        /*JNative call */
        JNative jnative = new JNative(libraryName, "MediaInfoA_Close");
        jnative.setParameter(0, Type.INT, handle);
        jnative.invoke();
    }


     /**
     * Create a memory pointer for giving it to an external library
     * @param value            The string to give
      * @return                 A pointer to the memory
    */
    static Pointer createPointer(String value) throws Exception
    {
        value+="\0";
        byte[] array=value.getBytes("UTF-8");
        Pointer valuePointer = new Pointer(MemoryBlockFactory.createMemoryBlock(array.length));
        valuePointer.setMemory(array);
        return valuePointer;
    }

     /**
     * Create a string from a memory pointer
     * @param jnative          The jnative handler
      * @return                 A string
    */
    static String retrieveString(JNative jnative) throws Exception
    {
        int address = Integer.parseInt(jnative.getRetVal());
        byte[]  strEnd          ={0};
        int     howFarToSearch  =10000;
        int     length          =0;

        while (true)
        {
            int pos=JNative.searchNativePattern(address+length, strEnd, howFarToSearch);
            if (pos == -1)
                howFarToSearch+=10000; //The strEnd wasn't found
            else
            {
                length+=pos;
                break;
            }
        }

        if (length > 0)
        {
            Pointer retPointer = new Pointer(new NativeMemoryBlock(address, length));
            String fileInfo = new String(retPointer.getMemory(), "UTF-8");
            retPointer.dispose();
            return fileInfo;
        }
        else
            return new String();
    }

    /**
     * Sets the default name of the library to be used.
     * If windows -> "MediaInfo.dll" else -> "libmediainfo.so.0"
     */
    public static void setLibraryName()
    {
        if (libraryName.equals(""))
        {
            String os=System.getProperty("os.name");
            if (os!=null && os.toLowerCase().startsWith("windows"))
                setLibraryName("MediaInfo.dll");
            else if (os!=null && os.toLowerCase().startsWith("mac"))
                setLibraryName("libmediainfo.dynlib.0");
            else
                setLibraryName("libmediainfo.so.0");
        }
    }
    

    /**
     * Sets the name of the library to be used.
     * @param libName            name of the library
     */
    public static void setLibraryName(String libName)
    {
        libraryName = libName;
    }
}


/**
 * Exception thrown if the handle isn't initialized.
 */
class HandleNotInitializedException extends Exception
{
    private static final long serialVersionUID = 1L;

    HandleNotInitializedException(String msg)
    {
        super(msg);
    }
}
