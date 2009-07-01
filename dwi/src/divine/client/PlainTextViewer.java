package divine.client;
import java.awt.Point;
import java.awt.event.*;
import javax.swing.event.*;
import javax.swing.*;
import javax.swing.text.DefaultCaret;
import divine.common.Logger;

public class PlainTextViewer extends javax.swing.JPanel
{

    protected class IgnorantCaret extends DefaultCaret {
        protected void adjustVisibility( java.awt.Rectangle r ) {
        }
    }

    public PlainTextViewer( String content, boolean caret )
    {
        m_scroll = new javax.swing.JScrollPane();
        m_text = new javax.swing.JTextArea();
        if ( !caret )
            m_text.setCaret( new IgnorantCaret() );
        /* ( ( DefaultCaret ) m_text.getCaret() ).setUpdatePolicy(
           DefaultCaret.UPDATE_WHEN_ON_EDT ); */
        setLayout( new java.awt.BorderLayout() );

        m_scroll.setViewportView( m_text );

        add( m_scroll, java.awt.BorderLayout.CENTER );
        setText( content );
    }

    public JTextArea textArea() { return m_text; }
    public void setText( String s ) {
        if ( s == null ) return;
        if ( s.equals( m_text.getText() ) )
            return;
        m_text.setText( s );
    }

    public String text() { return m_text.getText(); }

    private javax.swing.JScrollPane m_scroll;
    private javax.swing.JTextArea m_text;
}
