// -*- mode: java; c-basic-offset: 4 -*- 
package divine.client;
import divine.common.*;
import java.util.Map;
import java.util.regex.*;
import java.awt.Frame;
import java.io.*;
import javax.swing.DefaultComboBoxModel;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import java.awt.Component;

public class ModelEditor extends MainWindow.NodePanel
{
    protected Model m_model;
    protected Client m_client;
    public Model model() { return m_model; }

    public ModelEditor( Client c, Model m ) {
        m_client = c;
        initComponents();
        update( m );
    }

    public boolean update( Node elem ) {
        if ( !( elem instanceof Model ) )
            return false;

        Model m = m_model = (Model) elem;
        modelChanged();

        setEnabledRec( this, true );
        /* for ( Property p : model().properties().values() )
            for ( Task t : p.tasks().values() )
                if ( !t.mutable() )
                m_client.setEnabledRec( this, false ); */

        return true;
    }

    private void textChange( javax.swing.event.DocumentEvent evt )
    {
        model().setText( m_text.text() );
    }

    /* private void btnCheckSyntaxActionPerformed( java.awt.event.ActionEvent evt )
    {
        if ( !checkSyntax() )
        {
            JOptionPane.showMessageDialog( this,
                    "Model is not valid", "Invalid model", JOptionPane.ERROR_MESSAGE );
        }
        else
        {
            JOptionPane.showMessageDialog( this,
                    "Model is valid", "Valid", JOptionPane.INFORMATION_MESSAGE );
        }
        } */

    private boolean checkSyntax()
    {
        m_client.storeModel( model() );
        Map< String, Object > resp = m_client.request(
                "check-syntax", Wired.map( "model", model().name() ) );
        if ( resp.get( "error" ) != null ) {
            Pattern p = Pattern.compile( 
                    "([0-9]+)[^0-9]([0-9]+)[^0-9]([0-9]+)[^0-9]([0-9]+)" );
            Matcher m = p.matcher( (String) resp.get( "error" ) );
            if ( m.find() ) {
                Integer row = 1, col = 1, first = 0, last = 0;
                String s = model().text();
                int fr = Integer.valueOf( m.group( 1 ) ),
                    lr = Integer.valueOf( m.group( 3 ) ),
                    fc = Integer.valueOf( m.group( 2 ) ),
                    lc = Integer.valueOf( m.group( 4 ) );
                for ( int i = 0; i < s.length(); i++ ) {
                    if ( s.charAt( i ) == '\n' ) {
                        ++ row; col = 1;
                    } else ++ col;

                    if ( fr == row && fc == col )
                        first = i;
                    if ( lr == row && lc == col )
                        last = i;
                }
                Logger.log( 4, "selection: " + first.toString()
                        + ", " + last.toString() + ", length: " + s.length() );
                m_text.textArea().grabFocus();
                m_text.textArea().select( first, last + 1 );
            }
            return false;
        } else return true;
    }

    /* private void btnFileBrowserActionPerformed( java.awt.event.ActionEvent evt )
    {
        String[] ext;
        if ( model().type() == Model.Type.DVE )
        {
            ext = new String[ 2 ];
            ext[ 0 ] = "dve";
            ext[ 1 ] = "mdve";
        }
        else
        {
            ext = new String[ 1 ];
            ext[ 0 ] = "pml";
        }

        File f = m_client.mainWindow().chooseFile( ext );
        String content = m_client.readFile( f );
        if ( content != null )
        {
            if ( f.getName().endsWith( "mdve" ) )
            {
                //if user selected "mdve" file, let him change values of variables
                content = "mdve reading to be fixed";
                // MdveToDveDialog dlg = new MdveToDveDialog( cl, true, content );
                // dlg.setVisible( true );
                // if ( dlg.getReturnValue() == 1 )
                // {
                    // content = dlg.getConvertedModel();
                // }
            }
            model().setText( content );
            m_client.renameObject( model(),
                    f.getName().substring( 0, f.getName().lastIndexOf( '.' ) ) );
        }
        this.modelChanged();
        // XXX following font update may be redundant
        m_client.mainWindow().setFontRec( this, getFont() );
        } */

    public void modelChanged()
    {
        String x = "";
        if ( !model().text().equals( "<null>" ) )
            x = model().text();
        m_text.setText( x );
    }

    private javax.swing.JScrollPane m_scroll;
    private PlainTextViewer m_text;

    private void initComponents()
    {
        m_scroll = new javax.swing.JScrollPane();
        m_text = new PlainTextViewer( "", true );

        setLayout( new java.awt.BorderLayout() );

        m_scroll.setVerticalScrollBarPolicy(
                javax.swing.ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS );

        m_text.textArea().getDocument().addDocumentListener(
            new javax.swing.event.DocumentListener()
            {
                public void insertUpdate( javax.swing.event.DocumentEvent evt ) 
                { textChange( evt ); }
                public void removeUpdate( javax.swing.event.DocumentEvent evt ) 
                { textChange( evt ); }
                public void changedUpdate( javax.swing.event.DocumentEvent evt ) 
                { textChange( evt ); }
            }
        );

        add( m_text );

    }

}
