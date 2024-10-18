import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from matplotlib.colors import ListedColormap, BoundaryNorm


def lag_possible(entry_pitch, exit_pitch, shove_into_portal: float = 0):
    # flip to conventional 1D angle instead of source pitch
    entry_pitch *= -1
    exit_pitch *= -1
    entry_pitch_rad = np.deg2rad(entry_pitch)
    exit_pitch_rad = np.deg2rad(exit_pitch)
    entry_s = np.sin(entry_pitch_rad)
    entry_c = np.cos(entry_pitch_rad)
    exit_s = np.sin(exit_pitch_rad)
    exit_c = np.cos(exit_pitch_rad)
    entry_norm = entry_c, entry_s
    exit_norm = exit_c, exit_s

    if abs(entry_norm[1]) == 0:
        return False
    if abs(abs(entry_norm[1]) - 1) < 0.01 and abs(abs(exit_norm[1]) - 1) < 0.01:
        return False
    # assume that the portals & player center are at (0, 0) unless the player is shoved into the portal
    center = -np.array(entry_norm) * shove_into_portal
    origin = center - (0, 36)
    if entry_norm[1] > 0:
        center -= (0, 16)
    else:
        center += (0, 16)

    entry_mat = (
        (entry_c, -entry_s),
        (entry_s, entry_c),
    )
    exit_mat = (
        (exit_c, -exit_s),
        (exit_s, exit_c),
    )
    mirror_mat = ((-1, 0), (0, 1))

    tp_mat = np.matmul(exit_mat, np.matmul(mirror_mat, np.linalg.inv(entry_mat)))
    new_origin = np.matmul(tp_mat, center.reshape((2, 1))).reshape(2)
    new_origin += origin - center
    new_center = new_origin + (0, 18)
    return np.dot(new_center, exit_norm) <= 0


# print(lag_possible(90, -45))  # True
# ent_fire YEET newlocation "0 0 100 40 0 0"; ent_fire YEET2 newlocation "150 0 100 -10 0 0"
# print(lag_possible(40, -10))  # False
# print(lag_possible(40, -20))  # True


def do_plot():
    grid_res = np.linspace(-180, 180, 361)
    X, Y = np.meshgrid(grid_res, grid_res)
    units_into_portal = 0  # ~34 becomes impossible to do LAG/AAG
    Z = np.vectorize(lambda x, y: lag_possible(x, y, units_into_portal))(X, Y)

    cmap = ListedColormap(['black', 'pink'])
    norm = BoundaryNorm([0, 0.5, 1], cmap.N)

    # note the origin=lower it used to force the origin to be in the lower left
    plt.imshow(
        Z,
        interpolation='nearest',
        extent=[-180, 180, -180, 180],
        cmap=cmap,
        norm=norm,
        origin='lower',
    )
    plt.gca().add_patch(patches.Rectangle(
        (-90, -90), 180, 180,
        linewidth=1,
        edgecolor=(0, 1, 0),
        facecolor='none',
    ))
    cbar = plt.colorbar(ticks=[0, 1])
    cbar.ax.set_yticklabels(['not possible', 'possible'])

    lag_19 = np.array((90, -44.99942))
    aag_e01 = np.array((15.945395, -90))
    aag_e02 = np.array((60.141273, -90))
    ag_angles = np.array((lag_19, aag_e01, aag_e02))
    plt.scatter(ag_angles[:, 0], ag_angles[:, 1], color='red')
    plt.annotate(
        '19 LAG',
        xy=lag_19,
        textcoords='offset pixels',
        xytext=(30, 0),
        verticalalignment='center',
        arrowprops={'arrowstyle': '->'},
    )
    plt.annotate(
        'E01 AAG',
        xy=aag_e01,
        textcoords='offset pixels',
        xytext=(0, -30),
        horizontalalignment='left',
        arrowprops={'arrowstyle': '->'},
    )
    plt.annotate(
        'E02 AAG',
        xy=aag_e02,
        textcoords='offset pixels',
        xytext=(15, -45),
        horizontalalignment='left',
        arrowprops={'arrowstyle': '->'},
    )

    # plt.title(f'Player {units_into_portal} units behind portal plane (vel≈{units_into_portal / 0.015:.2f})')
    plt.title('Player at portal center')
    plt.xlabel('entry pitch')
    plt.ylabel('exit pitch')
    plt.show()


do_plot()
