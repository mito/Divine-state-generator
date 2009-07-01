package divine.client;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;

import divine.common.*;

/**
 * Panel used to show and change Dve Property, contains only one textarea.
 * @author  xforejt
 */

public class DvePropertyEditor extends MainWindow.NodePanel
{
    private Property m_property;
    public Property property() { return m_property; }

    public DvePropertyEditor( Client c, Property p )
    {
        initComponents();
        update( p );
    }

    public boolean update( Node e ) {
        if ( !( e instanceof Property ) )
            return false;
        m_property = (Property) e;
        String x = "";
        if ( property().text() != null && !property().text().equals( "<null>" ) )
            x = property().text();
        m_text.setText( x );
        return true;
    }

    private void textChange( javax.swing.event.DocumentEvent evt ) 
    {
        property().setText( m_text.text() );
    }

    private void initComponents()
    {
        m_text = new PlainTextViewer( "", true );

        setLayout( new java.awt.BorderLayout() );
        add( m_text );

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

    }

    protected PlainTextViewer m_text;
}
