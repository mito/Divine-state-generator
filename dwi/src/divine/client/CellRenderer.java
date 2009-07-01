// -*- mode: java; c-basic-offset: 4 -*- 
package divine.client;
import java.io.File;
import javax.swing.*;
import java.awt.Component;
import divine.common.*;
import javax.swing.tree.*;

public class CellRenderer extends DefaultTreeCellRenderer
{
    protected Icon icStar, icCross, icClock, icTick, icPromela, icDivine, icLTL,
        icSafety, icStateSpace, icStop;
    protected Icon stdOpenedIcon, stdClosedIcon, stdLeafIcon;
    protected MainWindow m_mainWindow;
    
    protected Icon loadIcon( String f ) {
        Logger.log( 3, "loading icon: " + m_mainWindow.client().imageDir() + "/" + f );
        return new ImageIcon( m_mainWindow.client().imageDir() + "/" + f );
    }

    public void loadIcons() {
        icStar = loadIcon( "star.gif" );
        icCross = loadIcon( "redcross.gif" );
        icClock = loadIcon( "clock.gif" );
        icTick = loadIcon( "tick.gif" );
        icPromela = loadIcon( "pml.gif" );
        icDivine = loadIcon( "dve.gif" );
        icLTL = loadIcon( "ltl.gif" );
        icStateSpace = loadIcon( "noprop.gif" );
        icSafety = loadIcon( "noprop.gif" );
        icStop = loadIcon( "stop.png" );

        stdOpenedIcon = getOpenIcon();
        stdClosedIcon = getClosedIcon();
        stdLeafIcon = getLeafIcon();
    }

    public CellRenderer( MainWindow w )
    {
        super();
        Logger.log( 2, "creating cell renderer" );
        m_mainWindow = w;
        loadIcons();
    }

    public Icon modelIcon( Model m ) {
        Model.Type mt = m.type();
        if ( mt == Model.Type.DVE )
            return icDivine;
        else if ( mt == Model.Type.PML )
            return icPromela;
        else return icStar; // question mark in fact
    }

    public Icon propertyIcon( Property p ) {
        Property.Type pt = p.type();
        if ( pt == Property.Type.LTL )
            return icLTL;
        else if ( pt == Property.Type.StateSpace)
            return icStateSpace;
        else if ( pt == Property.Type.Safety)
            return icSafety;
        else return modelIcon( p.model() );
    }

    public void setStdIcons() {
        setLeafIcon( stdLeafIcon );
        setOpenIcon( stdOpenedIcon );
        setClosedIcon( stdClosedIcon );
    }

    /**
     * Overriding method, ensures proper behaviour of this class. You don't have
     * to call it, Java machine calls it by itself when you assign cell renderer
     * to {@link javax.swing.JTree}
     */
    public Component getTreeCellRendererComponent(
            JTree tree, Object value, boolean sel,
            boolean expanded, boolean leaf, int row, boolean hasFocus )
    {
        if ( value instanceof DefaultMutableTreeNode ) {
            DefaultMutableTreeNode tn = ( DefaultMutableTreeNode ) value;
            Object o = tn.getUserObject();
            if ( o instanceof Node ) { 
                Node n = (Node) o;

                if ( n instanceof Job ) {
                    Job j = (Job) n;
                    switch ( j.state() ) {
                    case Prepared:
                    case Queued:
                    case NotStarted: setIcons( icStar ); break;
                    case Running: setIcons( icClock ); break;
                    case Finished:
                        if ( j.report().indexOf( "IsValid" ) == -1 ) {
			    if ( j.report().indexOf( "SafetyViolated:No" )!= -1 )
				setIcons( icTick );
			    else
				setIcons( icCross );
			}
                        else
                            if ( j.report().indexOf( "IsValid:Yes" ) != -1
                                    || j.report().indexOf( "IsValid:YES" ) != -1 )
                                setIcons( icTick );
                            else
                                setIcons( icCross );
                        break;
                    case Aborted:
                    case Aborting: setIcons( icStop ); break;
                    }
                }
                else if ( o instanceof Model )
                    setIcons( modelIcon( (Model) o ) );
                else if ( o instanceof Property )
                    setIcons( propertyIcon( (Property) o ) );
                else
                    setStdIcons();
            }
        }
        else
            setStdIcons();

        Component r = super.getTreeCellRendererComponent(
                tree, value,
                sel, expanded, leaf,
                row,
                hasFocus );
        
        Object o = ( ( DefaultMutableTreeNode ) value ).getUserObject();
        if ( o instanceof Node ) {
            setText( ( (Node) o ).name() );
        }

        return r;
    }

    /**
     * This class only calls classes setLeafIcon, setOpenIcon and setClosedIcon
     */
    private void setIcons( Icon i )
    {
        setLeafIcon( i );
        setOpenIcon( i );
        setClosedIcon( i );
    }
}
