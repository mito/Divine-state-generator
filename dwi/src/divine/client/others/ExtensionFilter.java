/*
* ExtensionFilter.java
*
* Created on 31. březen 2005, 9:19
*/

package divine.client.others;
import javax.swing.*;
import javax.swing.filechooser.*;
import java.io.File;

/**
 * Filter that accepts regular files with given extension. It also accepts all directories.
 * @author xforejt
 */
public class ExtensionFilter extends FileFilter {

    private String[] extensions;

    /** Creates a new instance of FilenameFilterOwner
     * @param extensions Array of allowed extensions
     */
    public ExtensionFilter( String[] extensions ) {
        this.extensions = extensions;
    }

    public boolean accept( File f )
    {
        for ( String s : this.extensions )
        {
            if ( f.isDirectory() || f.getName().endsWith( "." + s ) )
                return true;
        }
        return false;
    }

    public String getDescription()
    {
        return "Filter of files";
    }



}

